#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif
typedef u64 ptime_t;

#include "nouveau_pms.h"

#define NV04_PTIMER_TIME_0                                 0x00009400
#define NV04_PTIMER_TIME_1                                 0x00009410
#define NV04_PTIMER_CLOCK_DIV                              0x00009200

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
	return ((((ptime_t)high2) << 32) | (ptime_t)low);
}

static ptime_t
pms_launch(int cnum, struct pms_ucode* pms, ptime_t *wall_time)
{
	u32 pbus1098;
	u32 pms_data, pms_kick;
	ptime_t ptimer_start, ptimer_end;
	struct timeval wall_start, wall_end;
	int i;

	if (nva_cards[cnum].chipset < 0x90) {
		pms_data = 0x001400;
		pms_kick = 0x00000003;
	} else {
		pms_data = 0x080000;
		pms_kick = 0x00000001;
	}

	/* upload ucode */
	pbus1098 = nva_mask(cnum, 0x001098, 0x00000008, 0x00000000);
	nva_wr32(cnum, 0x001304, 0x00000000);
	for (i = 0; i < pms->len / 4; i++)
		nva_wr32(cnum, pms_data + (i * 4), pms->ptr.u32[i]);
	nva_wr32(cnum, 0x001098, pbus1098 | 0x18);

	/* and run it! */
	gettimeofday(&wall_start, NULL);
	ptimer_start = get_time(cnum);
	nva_wr32(cnum, 0x00130c, pms_kick);

	/* Wait for completion */
	while (nva_rd32(cnum, 0x001308) & 0x100);

	ptimer_end = get_time(cnum);
	gettimeofday(&wall_end, NULL);

	if (wall_time)
		*wall_time = time_diff_us(wall_start, wall_end);

	return ptimer_end - ptimer_start - (get_time(cnum) - get_time(cnum));
}

void timing_pms_waits(int cnum)
{
	struct pms_ucode _pms, *pms=&_pms;
	ptime_t exec_time;
	int instr, i;

	printf("Timing PMS wait instructions:\n");

	/* Slow PTIMER down */
	/*ptimer_div = nva_rd32(cnum, NV04_PTIMER_CLOCK_DIV);
	nva_wr32(cnum, NV04_PTIMER_CLOCK_DIV, ptimer_div * 2);*/

	for (instr=0; instr <= 0xf; instr++) {
		pms_init(pms);
		for (i = 0; i < 250; i++)
			pms_unkn(pms, instr);
		pms_fini(pms);
		exec_time = pms_launch(cnum, pms, NULL);
		printf("	Instr. 0x0%x: 0x%Lx PTIMER cycles\n", instr, exec_time/250);
	}

	/* Restore PTIMER */
	/*nva_wr32(cnum, NV04_PTIMER_CLOCK_DIV, ptimer_div);*/
}

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum =0;
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
	}

	timing_pms_waits(cnum);

	return 0;
}
