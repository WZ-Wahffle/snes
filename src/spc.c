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

void spc_reset(void) { spc.pc = spc_read_16(0xfffc); }

void spc_execute(void) {
    uint8_t opcode = spc_next_8();
    log_message(LOG_LEVEL_VERBOSE, "SPC fetched opcode 0x%02x", opcode);
    switch (opcode) {
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
