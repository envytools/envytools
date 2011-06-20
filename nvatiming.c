#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "nva.h"
#include "rnn.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif
typedef u64 ptime_t;
#include <time.h>

#define NV04_PTIMER_TIME_0                                 0x00009400
#define NV04_PTIMER_TIME_1                                 0x00009410

ptime_t time_diff_us(struct timeval start, struct timeval end)
{
	return ((end.tv_sec) - (start.tv_sec))*1000000 + ((end.tv_usec) - (start.tv_usec));
}

ptime_t get_time(unsigned int card)
{
	ptime_t low;

	/* From kmmio dumps on nv28 this looks like how the blob does this.
	* It reads the high dword twice, before and after.
	* The only explanation seems to be that the 64-bit timer counter
	* advances between high and low dword reads and may corrupt the
	* result. Not confirmed.
	*/
	ptime_t high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	ptime_t high1;
	do {
		high1 = high2;
		low = nva_rd32(card, NV04_PTIMER_TIME_0);
		high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	} while (high1 != high2);
	return ((((ptime_t)high2) << 32) | (ptime_t)low) >> 5;
}

void time_pcounter_nv10(unsigned int cnum)
{
	printf ("Perf counter:\n");


	nva_wr32(cnum, 0xa73c, 0);
	nva_wr32(cnum, 0xa404, 0);
	nva_wr32(cnum, 0xa40c, 0xffff);
	nva_wr32(cnum, 0xa414, 0xffff);
	nva_wr32(cnum, 0xa41c, 0);
	nva_wr32(cnum, 0xa620, 0);
	nva_wr32(cnum, 0xa624, 0);
	nva_wr32(cnum, 0xa628, 0);
	nva_wr32(cnum, 0xa62c, 0);
	if (nva_cards[cnum].card_type >= 0x20) {
		nva_wr32(cnum, 0xa504, 0);
		nva_wr32(cnum, 0xa50c, 0xffff);
		nva_wr32(cnum, 0xa514, 0xffff);
		nva_wr32(cnum, 0xa51c, 0);
		nva_wr32(cnum, 0xa720, 0);
		nva_wr32(cnum, 0xa724, 0);
		nva_wr32(cnum, 0xa728, 0);
		nva_wr32(cnum, 0xa72c, 0);
		nva_wr32(cnum, 0xa504, 0xffff);
	}
	nva_wr32(cnum, 0xa404, 0xffff);
	sleep(1);
	printf ("Set 0: %d Hz\n", nva_rd32(cnum, 0xa608));
	if (nva_cards[cnum].card_type >= 0x20) {
		printf ("Set 1: %d Hz\n", nva_rd32(cnum, 0xa708));
	}
}

void time_pcounter_nv84(unsigned int cnum)
{
	printf ("Perf counters:\n");

	int i;
	for (i = 0; i < 8; i++) {
		nva_wr32(cnum, 0xa7c0 + i * 4, 1);
		nva_wr32(cnum, 0xa420 + i * 4, 0xffff);
	}
		sleep(1);
	for (i = 0; i < 8; i++)
		nva_wr32(cnum, 0xa420 + i * 4, 0xffff);
	for (i = 0; i < 8; i++)
		printf ("Set %d: %d Hz\n", i, nva_rd32(cnum, 0xa600 + i * 4));
}

void time_pcounter_nvc0(unsigned int cnum)
{
	printf ("Perf counters:\n");

	int parts = nva_rd32(cnum, 0x121c74);
	int gpcs = nva_rd32(cnum, 0x121c78);

	int i, u;
	for (i = 0; i < 8; i++) {
		nva_wr32(cnum, 0x1b009c + i * 0x200, 0x40002);
		nva_wr32(cnum, 0x1b0044 + i * 0x200, 0xffff);
		nva_wr32(cnum, 0x1b00a8 + i * 0x200, 0);
	}
	sleep(1);
	for (i = 0; i < 8; i++)
		printf ("HUB set %d: %d Hz\n", i, nva_rd32(cnum, 0x1b00a8 + i * 0x200));
	for (u = 0; u < parts; u++) {
		for (i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x1a009c + i * 0x200 + u * 0x1000, 0x40002);
			nva_wr32(cnum, 0x1a0044 + i * 0x200 + u * 0x1000, 0xffff);
			nva_wr32(cnum, 0x1a00a8 + i * 0x200 + u * 0x1000, 0);
		}
		sleep(1);
		for (i = 0; i < 2; i++)
			printf ("ROPC[%d] set %d: %d Hz\n", u, i, nva_rd32(cnum, 0x1a00a8 + i * 0x200 + u * 0x1000));
	}
	for (u = 0; u < gpcs; u++) {
		for (i = 0; i < 1; i++) {
			nva_wr32(cnum, 0x18009c + i * 0x200 + u * 0x1000, 0x40002);
			nva_wr32(cnum, 0x180044 + i * 0x200 + u * 0x1000, 0xffff);
			nva_wr32(cnum, 0x1800a8 + i * 0x200 + u * 0x1000, 0);
		}
		sleep(1);
		for (i = 0; i < 1; i++)
			printf ("GPC[%d] set %d: %d Hz\n", u, i, nva_rd32(cnum, 0x1800a8 + i * 0x200 + u * 0x1000));
	}
}

void time_pgraph_dispatch_clock(unsigned int card)
{
	struct timeval start, end;
	ptime_t t_start, t_end;
	u32 reg;

	if (nva_cards[card].card_type == 0x50)
		reg = 0x4008f8;
	else if (nva_cards[card].card_type == 0xc0)
		reg = 0x4040f4;
	else {
		printf("pgraph_dispatch_clock is only available on nv50+ chipsets\n");
		return;
	}

	gettimeofday(&start, NULL);

	t_start = nva_rd32(card, reg);

	do
	{
		gettimeofday(&end, NULL);
	} while (time_diff_us(start, end) < 1000000);

	gettimeofday(&end, NULL);
	t_end = nva_rd32(card, reg);

	printf("PGRAPH.DISPATCH's clock: 1s = %llu cycles --> frequency = %f MHz\n",
	       (t_end - t_start), (t_end - t_start)/1000000.0);
}

ptime_t time_ptimer_clock(unsigned int card)
{
	struct timeval start, end;
	ptime_t t_start, t_end;

	gettimeofday(&start, NULL);
	t_start = get_time(card);

	do
	{
		gettimeofday(&end, NULL);
	} while (time_diff_us(start, end) < 1000000);

	gettimeofday(&end, NULL);
	t_end = get_time(card);

	return t_end - t_start;
}

u64 crystal_type(unsigned int card)
{
	unsigned int crystal, chipset;

	chipset = nva_cards[card].chipset;

	crystal = (nva_rd32(card, 0x101000) & 0x40) >> 6;

	if ((chipset >= 0x17 && chipset < 0x20) ||
		chipset > 0x25) {
		crystal += (nva_rd32(card, 0x101000) & 0x400000) >> 21;
	}

	switch (crystal) {
	case 0:
		return 13500000;
	case 1:
		return 14318800;
	case 2:
		return 27000000;
	case 3:
		return 25000000;
	default:
		return 0;
	}
}

void time_ptimer(unsigned int card)
{
	ptime_t ptimer_boot, ptimer_default, ptimer_max, ptimer_calibrated;

	/* Save the current values */
	u32 r9200 = nva_rd32(card, 0x9200);
	u32 r9210 = nva_rd32(card, 0x9210);
	u32 r9220 = nva_rd32(card, 0x9220);

	ptimer_boot = time_ptimer_clock(card);
	printf("PTIMER's boot: 1s = %llu cycles --> frequency = %f MHz\n",
	       ptimer_boot, ptimer_boot/1000000.0);

	/* Calibrate to standard frequency (27MHz?) */
	nva_wr32(card, 0x9200, 0x1);
	nva_wr32(card, 0x9210, 0x1);
	nva_wr32(card, 0x9220, 0x0);

	ptimer_default = time_ptimer_clock(card);
	printf("PTIMER's clock source: 1s = %llu cycles --> frequency = %f MHz\n",
	       ptimer_default, ptimer_default/1000000.0);

	if (nva_cards[card].card_type >= 0x40) {
		/* Calibrate to max frequency */
		nva_wr32(card, 0x9200, 0x1);
		nva_wr32(card, 0x9210, 0x1);
		nva_wr32(card, 0x9220, 0x10000);

		ptimer_max = time_ptimer_clock(card);
		printf("PTIMER's clock source (max): 1s = %llu cycles --> frequency = %f MHz\n",
		ptimer_max, ptimer_max/1000000.0);
	}

	/* Calibrate to read in µs */
	nva_wr32(card, 0x9200, 27);
	nva_wr32(card, 0x9210, 0x1);
	nva_wr32(card, 0x9220, 0x0);

	ptimer_calibrated = time_ptimer_clock(card);
	printf("PTIMER calibrated: 1s = %llu cycles --> frequency = %f MHz\n",
	       ptimer_calibrated, ptimer_calibrated/1000000.0);

	/* Restore the previous values */
	nva_wr32(card, 0x9200, r9200);
	nva_wr32(card, 0x9210, r9210);
	nva_wr32(card, 0x9220, r9220);
}

void time_fuc_engine_periodic (unsigned int card, const char *fuc_engine_name, u32 fucengine)
{
	struct timeval start, end;
	ptime_t t_start, t_end;

	/* Save the current values */
	u32 r24 = nva_rd32(card, fucengine + 0x24);
	u32 r28 = nva_rd32(card, fucengine + 0x28);

	nva_wr32(card, fucengine + 0x28, 0x1);
	nva_wr32(card, fucengine + 0x24, 0xffffffff);
	do
	{
		t_start = nva_rd32(card, fucengine + 0x24);
	} while (t_start < 10000000);

	gettimeofday(&start, NULL);

	while (t_start - nva_rd32(card, fucengine + 0x24) < 10000000);

	gettimeofday(&end, NULL);

	ptime_t val = time_diff_us(start, end);
	printf("%s's periodic timer: frequency = %f MHz\n", fuc_engine_name, 10000000.0/val);

	/* Restore the previous values */
	nva_wr32(card, fucengine + 0x28, r28);
	nva_wr32(card, fucengine + 0x24, r24);
}

void time_fuc_engine_watchdog (unsigned int card, const char *fuc_engine_name, u32 fucengine)
{
	struct timeval start, end;
	ptime_t t_start, t_end;

	/* Save the current values */
	u32 r34 = nva_rd32(card, fucengine + 0x34);
	u32 r38 = nva_rd32(card, fucengine + 0x38);

	nva_wr32(card, fucengine + 0x38, 0x1);
	nva_wr32(card, fucengine + 0x34, 0xffffffff);
	do
	{
		t_start = nva_rd32(card, fucengine + 0x34);
	} while (t_start < 10000000);

	gettimeofday(&start, NULL);

	while (t_start - nva_rd32(card, fucengine + 0x34) < 10000000);

	gettimeofday(&end, NULL);

	ptime_t val = time_diff_us(start, end);
	printf("%s's watchdog: frequency       = %f MHz\n", fuc_engine_name, 10000000.0/val);

	/* Restore the previous values */
	nva_wr32(card, fucengine + 0x38, r38);
	nva_wr32(card, fucengine + 0x34, r34);
}

int main(int argc, char **argv)
{
	struct nva_card *card = NULL;
	u32 pmc_enable;
	int c;
	char cnum = 0;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt (argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
		}

	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	} else
		card = &nva_cards[cnum];

	/* activate all the engines */
	pmc_enable = nva_rd32(cnum, 0x200);
	nva_wr32(cnum, 0x200, 0xffffffff);

	printf("Using card nv%x, crystal frequency = %f MHz\n\n",
	       card->chipset, crystal_type(cnum)/1000000.0);

	time_ptimer(cnum);
	printf("\n");

	time_pgraph_dispatch_clock(cnum);
	printf("\n");

	if (card->card_type == 0xc0) {
		time_pcounter_nvc0(cnum);
		printf("\n");
	} else if (card->chipset >= 0x84){
		time_pcounter_nv84(cnum);
		printf("\n");
	} else if (card->chipset >= 0x40) {
		printf("pcounter is unsupported on your card\n");
	} else if (card->chipset >= 0x10) {
		time_pcounter_nv10(cnum);
		printf("\n");
	}

	if (card->chipset < 0x98 || card->chipset == 0xa0) {
		/* restore PMC enable */
		nva_wr32(cnum, 0x200, pmc_enable);

		printf("Your card doesn't support fuc (chipset > nv98+ && chipset != nva0 needed)\n");
		return 0;
	}

	time_fuc_engine_periodic(cnum, "PBSP", 0x84000);
	time_fuc_engine_watchdog(cnum, "PBSP", 0x84000);
	printf("\n");

	time_fuc_engine_periodic(cnum, "PVP", 0x85000);
	time_fuc_engine_watchdog(cnum, "PVP", 0x85000);
	printf("\n");

	time_fuc_engine_periodic(cnum, "PPPP", 0x86000);
	time_fuc_engine_watchdog(cnum, "PPPP", 0x86000);
	printf("\n");

	if (card->chipset == 0x98 || card->chipset == 0xaa || card->chipset == 0xac) {
		time_fuc_engine_periodic(cnum, "PCRYPT", 0x87000);
		time_fuc_engine_watchdog(cnum, "PCRYPT", 0x87000);
		printf("\n");
	} else {
		printf("Your card doesn't support PCOPY (chipset < nva3 only)\n\n");
	}

	if (card->chipset >= 0xa3 && card->chipset != 0xaa && card->chipset != 0xac) {
		time_fuc_engine_periodic(cnum, "PCOPY[0]", 0x104000);
		time_fuc_engine_watchdog(cnum, "PCOPY[0]", 0x104000);
		printf("\n");
	} else {
		printf("Your card doesn't support PCOPY[0] (nva3+ only)\n\n");
	}

	if (card->chipset >= 0xc0) {
		time_fuc_engine_periodic(cnum, "PCOPY[1]", 0x105000);
		time_fuc_engine_watchdog(cnum, "PCOPY[1]", 0x105000);
		printf("\n");
	} else {
		printf("Your card doesn't support PCOPY[0] (nva3+ only)\n\n");
	}

	if (card->chipset >= 0xa3 && card->chipset != 0xaa && card->chipset != 0xac) {
		time_fuc_engine_periodic(cnum, "PDAEMON", 0x10a000);
		time_fuc_engine_watchdog(cnum, "PDAEMON", 0x10a000);
		printf("\n");
	} else {
		printf("Your card doesn't support PDAEMON (nva3+ only)\n\n");
	}

	if (card->chipset == 0xaf) {
		time_fuc_engine_periodic(cnum, "PUNK1C1", 0x1c1000);
		time_fuc_engine_watchdog(cnum, "PUNK1C1", 0x1c1000);
		printf("\n");
	} else {
		printf("Your card doesn't support PUNK1C1 (nvaf only)\n\n");
	}

	if (card->chipset >= 0xc0) {
		time_fuc_engine_periodic(cnum, "PGRAPH.CTXCTL", 0x409000);
		time_fuc_engine_watchdog(cnum, "PGRAPH.CTXCTL", 0x409000);
		printf("\n");

		time_fuc_engine_periodic(cnum, "PGRAPH.TP[0].CTXCTL", 0x502000);
		time_fuc_engine_watchdog(cnum, "PGRAPH.TP[0].CTXCTL", 0x502000);
		printf("\n");
	} else {
		printf("Your card doesn't support PGRAPH (nvc0+ only)\n\n");
	}

	/* restore PMC enable */
	nva_wr32(cnum, 0x200, pmc_enable);

	return 0;
}
