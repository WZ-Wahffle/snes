#include "spc.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

uint8_t spc_read_8(uint16_t addr) { return spc_mmu_read(addr); }

uint16_t spc_read_16(uint16_t addr) {
    uint8_t lsb = spc_read_8(addr);
    return TO_U16(lsb, spc_read_8(addr + 1));
}

uint8_t spc_next_8(void) { return spc_read_8(spc.pc++); }

void spc_write_8(uint16_t addr, uint8_t val) { spc_mmu_write(addr, val); }

void spc_write_16(uint16_t addr, uint16_t val) {
    spc_write_8(addr, U16_LOBYTE(val));
    spc_write_8(addr + 1, U16_HIBYTE(val));
}

void spc_reset(void) { spc.pc = spc_read_16(0xfffe); }

static uint8_t spc_cycle_counts[] = {
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 6,  8, 2, 8, 4, 5, 4, 5, 5, 6,
    5, 5, 6, 5, 2, 2, 4, 6, 2, 8, 4, 5, 3, 4, 3,  6, 2, 6, 5, 4, 5, 4, 5, 4,
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 3,  8, 2, 8, 4, 5, 3, 4, 3, 6,
    2, 6, 4, 4, 5, 4, 6, 6, 2, 8, 4, 5, 4, 5, 5,  6, 5, 5, 4, 5, 2, 2, 4, 3,
    2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 5,  5, 2, 8, 4, 5, 4, 5, 5, 6,
    5, 5, 5, 5, 2, 2, 3, 6, 2, 8, 4, 5, 3, 4, 3,  6, 2, 6, 5, 4, 5, 2, 4, 5,
    2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 12, 5, 3, 8, 4, 5, 3, 4, 3, 6,
    2, 6, 4, 4, 5, 2, 4, 4, 2, 8, 4, 5, 4, 5, 5,  6, 5, 5, 5, 5, 2, 2, 3, 4,
    3, 8, 4, 5, 4, 5, 4, 7, 2, 5, 6, 4, 5, 2, 4,  9, 2, 8, 4, 5, 5, 6, 6, 7,
    4, 5, 5, 5, 2, 2, 6, 3, 2, 8, 4, 5, 3, 4, 3,  6, 2, 4, 5, 3, 4, 3, 4, 3,
    2, 8, 4, 5, 4, 5, 5, 6, 3, 4, 5, 4, 2, 2, 5,  3,
};

void spc_execute(void) {
    uint8_t opcode = spc_next_8();
    log_message(LOG_LEVEL_VERBOSE, "SPC fetched opcode 0x%02x", opcode);
    // The SPC700 technically resides on its own clock, but this would make
    // synchronization awkward in emulation, so instead cycle counts are
    // multiplied by 21 which is roughly accurate to the lower clock frequency
    spc.remaining_clocks -= 21 * spc_cycle_counts[opcode];
    switch (opcode) {
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
