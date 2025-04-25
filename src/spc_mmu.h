#ifndef SPC_MMU_H_
#define SPC_MMU_H_

#include "types.h"

uint8_t spc_mmu_read(uint16_t addr);
void spc_mmu_write(uint16_t addr, uint8_t val);

#endif
