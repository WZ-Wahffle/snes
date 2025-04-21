#ifndef CPU_MMU_C_
#define CPU_MMU_C_

#include "types.h"

void mmu_init(memory_map_mode_t mode);
uint8_t mmu_read(uint16_t addr, uint8_t bank);
void mmu_write(uint16_t addr, uint8_t bank, uint8_t value);

#endif
