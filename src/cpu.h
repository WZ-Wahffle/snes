#ifndef CPU_H_
#define CPU_H_

#include "raylib.h"
#include "types.h"

uint8_t read_8(uint16_t addr, uint8_t bank);
uint16_t read_16(uint16_t addr, uint8_t bank);
uint32_t read_24(uint16_t addr, uint8_t bank);
void write_8(uint16_t addr, uint8_t bank, uint8_t val);
void write_16(uint16_t addr, uint8_t bank, uint16_t val);
void reset(void);
void execute(void);

#endif
