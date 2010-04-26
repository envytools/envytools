#include "rnn.h"
#include "rnndec.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

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
	int chdone = 0;
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
				}
				struct rnndecaddrinfo *ai = rnndec_decodeaddr(ctx, mmiodom, addr, line[0] == 'W');
				char *decoded_val = rnndec_decodeval(ctx, ai->typeinfo, value, ai->width);
				printf ("MMIO%d %s %s %s\n", width, ai->name, line[0]=='W'?"<=":"=>", decoded_val);
				free(ai->name);
				free(ai);
				free(decoded_val);
			} else if (bar1 && addr >= bar1 && addr < bar1+bar1l) {
				addr -= bar1;
				printf ("FB%d %"PRIx64" %s %"PRIx64"\n", width, addr, line[0]=='W'?"<=":"=>", value);
			} else if (bar2 && addr >= bar2 && addr < bar2+bar2l) {
				addr -= bar2;
				printf ("RAMIN%d %"PRIx64" %s %"PRIx64"\n", width, addr, line[0]=='W'?"<=":"=>", value);
			}
		} else {
			printf ("%s", line);
		}
	}
	return 0;
}
