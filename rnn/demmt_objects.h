#ifndef DEMMT_OBJECTS_H
#define DEMMT_OBJECTS_H

#include <stdint.h>

void demmt_parse_command(uint32_t class_, int curaddr, uint32_t cmd);
void demmt_object_init_chipset(int chipset);

#endif
