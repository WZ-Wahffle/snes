#include "cpu.h"
#include "cpu_mmu.h"
#include "instructions.h"
#include "types.h"

extern cpu_t cpu;

uint8_t read_8(uint16_t addr, uint8_t bank) { return mmu_read(addr, bank); }

uint16_t read_16(uint16_t addr, uint8_t bank) {
    return TO_U16(read_8(addr, bank), read_8(addr + 1, bank));
}

uint32_t read_24(uint16_t addr, uint8_t bank) {
    return TO_U24(read_16(addr, bank), read_8(addr + 2, bank));
}

void write_8(uint16_t addr, uint8_t bank, uint8_t val) {
    mmu_write(addr, bank, val);
}

void write_16(uint16_t addr, uint8_t bank, uint16_t val) {
    write_8(addr, bank, U16_LOBYTE(val));
    write_8(addr + 1, bank, U16_HIBYTE(val));
}

uint8_t next_8(void) { return read_8(cpu.pc++, cpu.pbr); }

uint16_t next_16(void) {
    uint8_t lsb = next_8();
    return TO_U16(lsb, next_8());
}

uint32_t next_24(void) {
    uint16_t lss = next_16();
    return TO_U24(lss, next_8());
}

void set_status_bit(status_bit_t bit, bool value) {
    if (value) {
        cpu.p |= 1 << bit;
    } else {
        cpu.p &= ~(1 << bit);
    }
}

bool get_status_bit(status_bit_t bit) {
    return cpu.p & (1 << bit);
}

uint32_t resolve_addr(addressing_mode_t mode) {
    uint32_t ret;
    switch (mode) {
    case AM_ABS:
        ret = TO_U24(next_16(), cpu.dbr);
        break;
    case AM_INDX:
        // only to be used with JMP instructions, must be dereferenced for the
        // actual value
        ret = TO_U24(next_16() + cpu.x, cpu.pbr);
        break;
    case AM_ABSX:
        ret = TO_U24(next_16() + cpu.x, cpu.dbr);
        break;
    case AM_ABSY:
        ret = TO_U24(next_16() + cpu.y, cpu.dbr);
        break;
    case AM_IND:
        // only to be used with JMP instructions, must be dereferenced for the
        // actual value
        ret = next_16();
        break;
    case AM_ABSX_L:
        ret = next_24() + cpu.x;
        break;
    case AM_ABS_L:
        ret = next_24();
        break;
    case AM_INDX_DIR:
        ret = TO_U24(read_16(next_8() + cpu.d + cpu.x, 0), cpu.dbr);
        break;
    case AM_ZBKX_DIR:
        ret = TO_U24(next_8() + cpu.d + cpu.x, 0);
        break;
    case AM_ZBKY_DIR:
        ret = TO_U24(next_8() + cpu.d + cpu.y, 0);
        break;
    case AM_INDY_DIR:
        ret = TO_U24(read_16(next_8() + cpu.d, 0), cpu.dbr) + cpu.y;
        break;
    case AM_INDY_DIR_L:
        ret = read_24(next_8() + cpu.d, 0) + cpu.y;
        break;
    case AM_IND_DIR_L:
        ret = read_24(next_8() + cpu.d, 0);
        break;
    case AM_IND_DIR:
        ret = TO_U24(read_16(next_8() + cpu.d, 0), cpu.dbr);
        break;
    case AM_DIR:
        ret = next_8() + cpu.d;
        break;
    case AM_PC_REL_L:
        ret = cpu.pc + (int16_t)next_16() + 2;
        break;
    case AM_PC_REL:
        ret = cpu.pc + (int8_t)next_8() + 1;
        break;
    case AM_STK_REL:
        ret = TO_U24(next_8() + cpu.s, 0);
        break;
    case AM_STK_REL_INDY:
        ret = TO_U24(read_16(next_8() + cpu.s, 0), cpu.dbr) + cpu.y;
        break;
    default:
        UNREACHABLE_SWITCH(mode);
    }

    return ret;
}

void reset(void) { cpu.pc = read_16(0xfffc, 0); }

void execute(void) {
    uint8_t opcode = next_8();
    log_message(LOG_LEVEL_VERBOSE, "fetched opcode 0x%02x", opcode);
    switch (opcode) {
    case 0x78:
        sei(AM_IMP);
        break;
    case 0x9c:
        stz(AM_ABS);
        break;
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
