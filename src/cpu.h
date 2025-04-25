#ifndef CPU_H_
#define CPU_H_

#include "cpu_mmu.h"
#include "cpu_instructions.h"
#include "types.h"

void cpu_reset(void);
void cpu_execute(void);

#endif
