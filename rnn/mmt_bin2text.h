#ifndef MMT_BIN2TEXT_H_
#define MMT_BIN2TEXT_H_

#include <stdint.h>

#define PRINT_DATA 1

#define BUF_SIZE 64 * 1024
extern unsigned char buf[BUF_SIZE];
extern int idx;

#define __packed  __attribute__((__packed__))
#define PFX "--0000-- "

struct mmt_message
{
	uint8_t type;
} __packed;

struct mmt_buf
{
	uint32_t len;
	uint8_t data[0];
} __packed;

void *load_data(int sz);
void check_eor(uint8_t e);
void dump_next();
void parse_nvidia(void);


#endif /* MMT_BIN2TEXT_H_ */
