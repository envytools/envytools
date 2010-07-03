#include "rnn.h"
#include "rnndec.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

int chdone = 0;
int arch = 0;
uint64_t praminbase = 0;
uint64_t ramins = 0;
uint64_t fakechan = 0;
int i2cip = -1;

struct mpage {
	uint64_t tag;
	uint32_t contents[0x1000/4];
};

struct mpage **pages = 0;
int pagesnum = 0, pagesmax = 0;

uint32_t *findmem (uint64_t addr) {
	int i;
	uint64_t tag = addr & ~0xfffull;
	for (i = 0; i < pagesnum; i++) {
		if (tag == pages[i]->tag)
			return &pages[i]->contents[(addr&0xfff)/4];
	}
	struct mpage *pg = calloc (sizeof *pg, 1);
	pg->tag = tag;
	RNN_ADDARRAY(pages, pg);
	return &pg->contents[(addr&0xfff)/4];
}

struct i2c_ctx {
	int last;
	int aok;
	int wr;
	int bits;
	int b;
	int pend;
} i2cb[10] = { { 0 } };

int i2c_bus_num (uint64_t addr) {
	switch (addr) {
		case 0xe138:
			return 0;
		case 0xe150:
			return 1;
		case 0xe168:
			return 2;
		case 0xe180:
			return 3;
		case 0xe254:
			return 4;
		case 0xe174:
			return 5;
		case 0xe764:
			return 6;
		case 0xe780:
			return 7;
		case 0xe79c:
			return 8;
		case 0xe7b8:
			return 9;
		default:
			return -1;
	}
}

void doi2cr (struct i2c_ctx *ctx, int byte) {
	if (ctx->pend) {
		if (ctx->bits == 8) {
			if (byte & 2)
				printf ("- ");
			else
				printf ("+ ");
			if (!ctx->aok) {
				ctx->aok = 1;
				ctx->wr = !(ctx->b&1);
			}
			ctx->bits = 0;
			ctx->b = 0;
		} else {
			ctx->b <<= 1;
			ctx->b |= (byte & 2) >> 1;
			ctx->bits++;
			if (ctx->bits == 8) {
				printf (">%02x", ctx->b);
			}
		}
		ctx->pend = 0;
	}
}

void doi2cw (struct i2c_ctx *ctx, int byte) {
	if (!ctx->last)
		ctx->last = 7;
	if (!(byte & 1) && (ctx->last & 1)) {
		/* clock went low */
	}
	if ((byte & 1) && !(ctx->last & 1)) {
		if (ctx->pend) {
			printf ("\nI2C LOST!\n");
			doi2cr(ctx, 0);
			ctx->pend = 0;
		}
		/* clock went high */
		if (ctx->bits == 8) {
			if (ctx->wr || !ctx->aok) {
				ctx->pend = 1;
			} else {
				if (byte & 2)
					printf ("- ");
				else
					printf ("+ ");
				ctx->bits = 0;
				ctx->b = 0;
			}
		} else {
			if (ctx->wr || !ctx->aok) {
				ctx->b <<= 1;
				ctx->b |= (byte & 2) >> 1;
				ctx->bits++;
				if (ctx->bits == 8) {
					printf ("<%02x", ctx->b);
				}
			} else {
				ctx->pend = 1;
			}
		}
	}
	if ((byte & 1) && !(byte & 2) && (ctx->last & 2)) {
		/* data went low with high clock - start bit */
		printf ("START ");
		ctx->bits = 0;
		ctx->b = 0;
		ctx->aok = 0;
		ctx->wr = 0;
		ctx->pend = 0;
	}
	if ((byte & 1) && (byte & 2) && !(ctx->last & 2)) {
		/* data went high with high clock - stop bit */
		printf ("STOP\n");
		i2cip = -1;
		ctx->bits = 0;
		ctx->b = 0;
		ctx->aok = 0;
		ctx->wr = 0;
		ctx->pend = 0;
	}
	ctx->last = byte;
}

int main(int argc, char **argv) {
	rnn_init();
	if (argc < 3) {
		printf ("Usage:\n"
				"\tdemmio file.xml trace.txt\n"
			);
		return 0;
	}
	struct rnndb *db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_prepdb (db);
	struct rnndeccontext *ctx = rnndec_newcontext(db);
	ctx->colors = &rnndec_colorsterm;
	struct rnndomain *mmiodom = rnn_finddomain(db, "NV_MMIO");
	FILE *fin = fopen(argv[2], "r");
	char line[1024];
	uint64_t bar0 = 0, bar0l, bar1, bar1l, bar2, bar2l;
	int i;
	for (i = 0; i < 10; i++)
		i2cb[i].last = 7;
	while (1) {
		/* yes, static buffer. but mmiotrace lines are bound to have sane length anyway. */
		if (!fgets(line, sizeof(line), fin))
			break;
		if (!strncmp(line, "PCIDEV ", 7)) {
			uint64_t bar[4], len[4], pciid;
			sscanf (line, "%*s %*s %"SCNx64" %*s %"SCNx64" %"SCNx64" %"SCNx64" %"SCNx64" %*s %*s %*s %"SCNx64" %"SCNx64" %"SCNx64" %"SCNx64"", &pciid, &bar[0], &bar[1], &bar[2], &bar[3], &len[0], &len[1], &len[2], &len[3]);
			if ((pciid >> 16) == 0x10de && bar[0] && (bar[0] & 0xf) == 0 && bar[1] && (bar[1] & 0xf) == 0xc) {
				bar0 = bar[0], bar0l = len[0];
				bar1 = bar[1], bar1l = len[1];
				if (bar[2])
					bar2 = bar[2], bar2l = len[2];
				else
					bar2 = bar[3], bar2l = len[3];
				bar0 &= ~0xf;
				bar1 &= ~0xf;
				bar2 &= ~0xf;
			}
			printf ("%s", line);
		} else if (!strncmp(line, "W ", 2) || !strncmp(line, "R ", 2)) {
			int skip = 0;
			uint64_t addr, value;
			int width;
			sscanf (line, "%*s %d %*s %*d %"SCNx64" %"SCNx64, &width, &addr, &value);
			width *= 8;
			if (bar0 && addr >= bar0 && addr < bar0+bar0l) {
				addr -= bar0;
				if (addr == 0 && !chdone) {
					char chname[5];
					if (value & 0x0f000000)
						snprintf(chname, 5, "NV%02"PRIX64, (value >> 20) & 0xff);
					else if (value & 0x0000f000)
						snprintf(chname, 5, "NV%02"PRIX64, ((value >> 20) & 0xf) + 4);
					else
						snprintf(chname, 5, "NV%02"PRIX64, ((value >> 16) & 0xf));
					rnndec_varadd(ctx, "chipset", chname);
					switch ((value >> 20) & 0xf0) {
						case 0:
							arch = 0;
							break;
						case 0x10:
							arch = 1;
							break;
						case 0x20:
							arch = 2;
							break;
						case 0x30:
							arch = 3;
							break;
						case 0x40:
						case 0x60:
							arch = 4;
							break;
						case 0x50:
						case 0x80:
						case 0x90:
						case 0xa0:
							arch = 5;
							break;
						case 0xc0:
							arch = 6;
							break;
					}
				} else if (arch >= 5 && addr == 0x1700) {
					praminbase = value << 16;
				} else if (arch == 5 && addr == 0x1704) {
					fakechan = (value & 0xfffffff) << 12;
				} else if (arch == 5 && addr == 0x170c) {
					ramins = (value & 0xffff) << 4;
				} else if (arch >= 6 && addr == 0x1714) {
					ramins = (value & 0xfffffff) << 12;
				} else if (arch >= 5 && (addr & 0xfff000) == 0xe000) {
					int bus = i2c_bus_num(addr);
					if (bus != -1) {
						if (i2cip != bus) {
							if (i2cip != -1)
								printf ("\n");
							struct rnndecaddrinfo *ai = rnndec_decodeaddr(ctx, mmiodom, addr, line[0] == 'W');
							printf ("I2C      0x%06"PRIx64"            %s ", addr, ai->name);
							free(ai->name);
							free(ai);
							i2cip = bus;
						}
						if (line[0] == 'R') {
							doi2cr(&i2cb[bus], value);
						} else {
							doi2cw(&i2cb[bus], value);
						}
						skip = 1;
					}
				} else if ((addr & 0xfff000) == 0x9000 && (i2cip != -1)) {
					/* ignore PTIMER meddling during I2C */
					skip = 1;
				}
				if (!skip && (i2cip != -1)) {
					printf ("\n");
					i2cip = -1;
				}
				if (arch >= 5 && addr >= 0x700000 && addr < 0x800000) {
					addr -= 0x700000;
					addr += praminbase;
					printf ("MEM%d %"PRIx64" %s %"PRIx64"\n", width, addr, line[0]=='W'?"<=":"=>", value);
					*findmem(addr) = value;
				} else if (!skip) {
					struct rnndecaddrinfo *ai = rnndec_decodeaddr(ctx, mmiodom, addr, line[0] == 'W');
					char *decoded_val = rnndec_decodeval(ctx, ai->typeinfo, value, ai->width);
					printf ("MMIO%d %c 0x%06"PRIx64" 0x%08"PRIx64" %s %s %s\n", width, line[0], addr, value, ai->name, line[0]=='W'?"<=":"=>", decoded_val);
					free(ai->name);
					free(ai);
					free(decoded_val);
				}
			} else if (bar1 && addr >= bar1 && addr < bar1+bar1l) {
				addr -= bar1;
				printf ("FB%d %"PRIx64" %s %"PRIx64"\n", width, addr, line[0]=='W'?"<=":"=>", value);
			} else if (bar2 && addr >= bar2 && addr < bar2+bar2l) {
				addr -= bar2;
				if (arch >= 6) {
					uint64_t pd = *findmem(ramins + 0x200);
					uint64_t pt = *findmem(pd + 4);
					pt &= 0xfffffff0;
					pt <<= 8;
					uint64_t pg = *findmem(pt + (addr/0x1000) * 8);
					pg &= 0xfffffff0;
					pg <<= 8;
					pg += (addr&0xfff);
					*findmem(pg) = value;
//					printf ("%"PRIx64" %"PRIx64" %"PRIx64" %"PRIx64"\n", ramins, pd, pt, pg);
					printf ("RAMIN%d %"PRIx64" %"PRIx64" %s %"PRIx64"\n", width, addr, pg, line[0]=='W'?"<=":"=>", value);
				} else if (arch == 5) {
					uint64_t paddr = addr;
					paddr += *findmem(fakechan + ramins + 8);
					paddr += (uint64_t)(*findmem(fakechan + ramins + 12) >> 24) << 32;
					uint64_t pt = *findmem(fakechan + 0x200 + ((paddr >> 29) << 3));
//					printf ("%#"PRIx64" PT: %#"PRIx64" %#"PRIx64" ", paddr, fakechan + 0x200 + ((paddr >> 29) << 3), pt);
					uint32_t div = (pt & 2 ? 0x1000 : 0x10000);
					pt &= 0xfffff000;
					uint64_t pg = *findmem(pt + ((paddr&0x1ffff000)/div) * 8);
					uint64_t pgh = *findmem(pt + ((paddr&0x1ffff000)/div) * 8 + 4);
//					printf ("PG: %#"PRIx64" %#"PRIx64"\n", pt + ((paddr&0x1ffff000)/div) * 8, pgh << 32 | pg);
					pg &= 0xfffff000;
					pg |= (pgh & 0xff) << 32;
					pg += (paddr & (div-1));
					*findmem(pg) = value;
//					printf ("%"PRIx64" %"PRIx64" %"PRIx64" %"PRIx64"\n", ramins, pd, pt, pg);
					printf ("RAMIN%d %"PRIx64" %"PRIx64" %s %"PRIx64"\n", width, addr, pg, line[0]=='W'?"<=":"=>", value);
				} else {
					printf ("RAMIN%d %"PRIx64" %s %"PRIx64"\n", width, addr, line[0]=='W'?"<=":"=>", value);
				}
			}
		} else {
			printf ("%s", line);
		}
	}
	return 0;
}
