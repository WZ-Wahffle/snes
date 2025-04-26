#include "cpu.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

uint16_t read_r(r_t reg) {
    switch (reg) {
    case R_C:
        if (cpu.emulation_mode || get_status_bit(STATUS_MEMNARROW)) {
            return U16_LOBYTE(cpu.c);
        }
        return cpu.c;
    case R_X:
        if (cpu.emulation_mode || get_status_bit(STATUS_XNARROW)) {
            return U16_LOBYTE(cpu.x);
        }
        return cpu.x;
    case R_Y:
        if (cpu.emulation_mode || get_status_bit(STATUS_XNARROW)) {
            return U16_LOBYTE(cpu.y);
        }
        return cpu.y;
    case R_S:
        return cpu.s;
    case R_D:
        return cpu.d;
        break;
    default:
        UNREACHABLE_SWITCH(reg);
    }
}

uint8_t read_8(uint16_t addr, uint8_t bank) { return mmu_read(addr, bank, true); }

uint8_t read_8_no_log(uint16_t addr, uint8_t bank) { return mmu_read(addr, bank, false); }

uint16_t read_16(uint16_t addr, uint8_t bank) {
    return TO_U16(read_8(addr, bank), read_8(addr + 1, bank));
}

uint32_t read_24(uint16_t addr, uint8_t bank) {
    return TO_U24(read_16(addr, bank), read_8(addr + 2, bank));
}

void write_r(r_t reg, uint16_t val) {
    switch (reg) {
    case R_C:
        if (cpu.emulation_mode || get_status_bit(STATUS_MEMNARROW)) {
            val = U16_LOBYTE(val);
            cpu.c &= 0xff00;
        } else {
            cpu.c &= 0;
        }
        cpu.c |= val;
        break;
    case R_X:
        if (cpu.emulation_mode || get_status_bit(STATUS_XNARROW)) {
            val = U16_LOBYTE(val);
            cpu.x &= 0xff00;
        } else {
            cpu.x &= 0;
        }
        cpu.x |= val;
        break;
    case R_Y:
        if (cpu.emulation_mode || get_status_bit(STATUS_XNARROW)) {
            val = U16_LOBYTE(val);
            cpu.y &= 0xff00;
        } else {
            cpu.y &= 0;
        }
        cpu.y |= val;
        break;
    case R_S:
        cpu.s = val;
        break;
    case R_D:
        cpu.d = val;
        break;
    }
}

void write_8(uint16_t addr, uint8_t bank, uint8_t val) {
    mmu_write(addr, bank, val, true);
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
    log_message(LOG_LEVEL_VERBOSE, "CPU set status bit %d", bit);
    if (value) {
        cpu.p |= 1 << bit;
    } else {
        cpu.p &= ~(1 << bit);
    }
    if (cpu.emulation_mode) {
        cpu.p |= 0b110000;
    }
}

bool get_status_bit(status_bit_t bit) {
    if (cpu.emulation_mode) {
        cpu.p |= 0b110000;
    }
    return cpu.p & (1 << bit);
}

void push_8(uint8_t val) { write_8(cpu.s--, 0, val); }
void push_16(uint16_t val) {
    push_8(U16_HIBYTE(val));
    push_8(U16_LOBYTE(val));
}
void push_24(uint32_t val) {
    push_8(U24_HIBYTE(val));
    push_16(U24_LOSHORT(val));
}
uint8_t pop_8(void) { return read_8(++cpu.s, 0); }
uint16_t pop_16(void) {
    uint8_t lsb = pop_8();
    return TO_U16(lsb, pop_8());
}
uint32_t pop_24(void) {
    uint16_t lss = pop_16();
    return TO_U24(lss, pop_8());
}

uint32_t resolve_addr(addressing_mode_t mode) {
    uint32_t ret;
    switch (mode) {
    case AM_ABS:
        ret = TO_U24(next_16(), cpu.dbr);
        break;
    case AM_INDX:
        // only to be used with JMP instructions, must be
        // dereferenced for the actual value
        ret = TO_U24(next_16() + read_r(R_X), cpu.pbr);
        break;
    case AM_ABSX:
        ret = TO_U24(next_16() + read_r(R_X), cpu.dbr);
        break;
    case AM_ABSY:
        ret = TO_U24(next_16() + read_r(R_Y), cpu.dbr);
        break;
    case AM_IND:
        // only to be used with JMP instructions, must be
        // dereferenced for the actual value
        ret = next_16();
        break;
    case AM_ABSX_L:
        ret = next_24() + read_r(R_X);
        break;
    case AM_ABS_L:
        ret = next_24();
        break;
    case AM_INDX_DIR:
        ret = TO_U24(read_16(next_8() + read_r(R_D) + read_r(R_X), 0), cpu.dbr);
        break;
    case AM_ZBKX_DIR:
        ret = TO_U24(next_8() + read_r(R_D) + read_r(R_X), 0);
        break;
    case AM_ZBKY_DIR:
        ret = TO_U24(next_8() + read_r(R_D) + read_r(R_Y), 0);
        break;
    case AM_INDY_DIR:
        ret = TO_U24(read_16(next_8() + read_r(R_D), 0), cpu.dbr) + read_r(R_Y);
        break;
    case AM_INDY_DIR_L:
        ret = read_24(next_8() + read_r(R_D), 0) + read_r(R_Y);
        printf("0x%06x\n", ret);
        break;
    case AM_IND_DIR_L:
        ret = read_24(next_8() + read_r(R_D), 0);
        break;
    case AM_IND_DIR:
        ret = TO_U24(read_16(next_8() + read_r(R_D), 0), cpu.dbr);
        break;
    case AM_DIR:
        ret = next_8() + read_r(R_D);
        break;
    case AM_PC_REL_L:
        ret = cpu.pc + (int16_t)next_16() + 2;
        break;
    case AM_PC_REL:
        ret = cpu.pc + (int8_t)next_8() + 1;
        break;
    case AM_STK_REL:
        ret = TO_U24(next_8() + read_r(R_S), 0);
        break;
    case AM_STK_REL_INDY:
        ret = TO_U24(read_16(next_8() + read_r(R_S), 0), cpu.dbr) + read_r(R_Y);
        break;
    default:
        UNREACHABLE_SWITCH(mode);
    }

    return ret;
}

uint16_t resolve_read16(addressing_mode_t mode, bool respect_x,
                        bool respect_m) {
    switch (mode) {
    case AM_ABS:
    case AM_INDX:
    case AM_ABSX:
    case AM_ABSY:
    case AM_IND:
    case AM_ABSX_L:
    case AM_ABS_L:
    case AM_INDX_DIR:
    case AM_ZBKX_DIR:
    case AM_ZBKY_DIR:
    case AM_INDY_DIR:
    case AM_INDY_DIR_L:
    case AM_IND_DIR_L:
    case AM_IND_DIR:
    case AM_DIR:
    case AM_PC_REL_L:
    case AM_STK_REL:
    case AM_STK_REL_INDY: {
        uint32_t addr = resolve_addr(mode);
        return read_16(U24_LOSHORT(addr), U24_HIBYTE(addr));
    }
    case AM_ACC:
        return read_r(R_C);
    case AM_IMM:
        if ((get_status_bit(STATUS_XNARROW) && respect_x) ||
            (get_status_bit(STATUS_MEMNARROW) && respect_m)) {
            return next_8();
        } else {
            return next_16();
        }
    default:
        UNREACHABLE_SWITCH(mode);
    }
}

uint16_t resolve_read8(addressing_mode_t mode) {
    switch (mode) {
    case AM_IMM:
        return next_8();
    default:
        UNREACHABLE_SWITCH(mode);
    }
}

void cpu_reset(void) {
    cpu.pc = read_16(0xfffc, 0);
    cpu.emulation_mode = true;
    cpu.p = 0b110000;
}

static uint8_t cpu_cycle_counts[] = {
    7, 6, 7, 4, 5, 3, 5, 6, 3, 2, 2, 4, 6, 4, 6, 5, 2, 5, 5, 7, 5, 4, 6, 6,
    2, 4, 2, 2, 6, 4, 7, 5, 6, 6, 8, 4, 3, 3, 5, 6, 4, 2, 2, 5, 4, 4, 6, 5,
    2, 5, 5, 7, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 5, 7, 6, 2, 4, 7, 3, 5, 6,
    3, 2, 2, 3, 3, 4, 6, 5, 2, 5, 5, 7, 7, 4, 6, 6, 2, 4, 3, 2, 4, 4, 7, 5,
    6, 6, 6, 4, 3, 3, 5, 6, 4, 2, 2, 6, 5, 4, 6, 5, 2, 5, 5, 7, 4, 4, 6, 6,
    2, 4, 4, 2, 6, 4, 7, 5, 2, 6, 4, 4, 3, 3, 3, 2, 2, 2, 2, 3, 4, 4, 4, 5,
    2, 6, 5, 7, 4, 4, 4, 6, 2, 5, 2, 2, 4, 5, 5, 5, 2, 6, 2, 4, 3, 3, 3, 6,
    2, 2, 2, 4, 4, 4, 4, 5, 2, 5, 5, 7, 4, 4, 4, 6, 2, 4, 2, 2, 4, 4, 4, 5,
    2, 6, 3, 4, 3, 3, 5, 6, 2, 2, 2, 3, 4, 4, 6, 5, 2, 5, 5, 7, 6, 4, 6, 6,
    2, 4, 3, 3, 6, 4, 7, 5, 2, 6, 3, 4, 3, 3, 5, 6, 2, 2, 2, 3, 4, 4, 6, 5,
    2, 5, 5, 7, 5, 4, 6, 6, 2, 4, 4, 2, 8, 4, 7, 5,
};

void cpu_execute(void) {
    uint8_t opcode = next_8();
    log_message(LOG_LEVEL_VERBOSE, "CPU fetched opcode 0x%02x", opcode);
    // TODO: disambiguate cpu cycles taking 6, 8 or 12 clock cycles

    // remaining clocks increments at 341 * 262 * 60 * 4 = 21.44MHz.
    // The highest achievable CPU clock speed is roughly 3.58 MHz,
    // which corresponds to 6 master clocks per CPU clock, hence the
    // factor of 6
    cpu.remaining_clocks -= 6 * cpu_cycle_counts[opcode];
    switch (opcode) {
    case 0x08:
        php(AM_STK);
        break;
    case 0x10:
        bpl(AM_PC_REL);
        break;
    case 0x18:
        clc(AM_IMP);
        break;
    case 0x1a:
        inc(AM_ACC);
        break;
    case 0x1b:
        tcs(AM_IMP);
        break;
    case 0x20:
        jsr(AM_ABS);
        break;
    case 0x28:
        plp(AM_STK);
        break;
    case 0x2a:
        rol(AM_ACC);
        break;
    case 0x38:
        sec(AM_IMP);
        break;
    case 0x48:
        pha(AM_STK);
        break;
    case 0x58:
        cli(AM_IMP);
        break;
    case 0x5b:
        tcd(AM_IMP);
        break;
    case 0x60:
        rts(AM_IMP);
        break;
    case 0x68:
        pla(AM_STK);
        break;
    case 0x69:
        adc(AM_IMM);
        break;
    case 0x70:
        bvs(AM_PC_REL);
        break;
    case 0x78:
        sei(AM_IMP);
        break;
    case 0x80:
        bra(AM_PC_REL);
        break;
    case 0x88:
        dey(AM_IMP);
        break;
    case 0x8d:
        sta(AM_ABS);
        break;
    case 0x8f:
        sta(AM_ABS_L);
        break;
    case 0x98:
        tya(AM_ACC);
        break;
    case 0x9c:
        stz(AM_ABS);
        break;
    case 0x9f:
        sta(AM_ABSX_L);
        break;
    case 0xa0:
        ldy(AM_IMM);
        break;
    case 0xa2:
        ldx(AM_IMM);
        break;
    case 0xa8:
        tay(AM_IMP);
        break;
    case 0xa9:
        lda(AM_IMM);
        break;
    case 0xaa:
        tax(AM_IMP);
        break;
    case 0xb7:
        lda(AM_INDY_DIR_L);
        break;
    case 0xc2:
        rep(AM_IMM);
        break;
    case 0xc8:
        iny(AM_IMP);
        break;
    case 0xca:
        dex(AM_IMP);
        break;
    case 0xcd:
        cmp(AM_ABS);
        break;
    case 0xd0:
        bne(AM_PC_REL);
        break;
    case 0xe0:
        cpx(AM_IMM);
        break;
    case 0xe2:
        sep(AM_IMM);
        break;
    case 0xe8:
        inx(AM_IMP);
        break;
    case 0xe9:
        sbc(AM_IMM);
        break;
    case 0xeb:
        xba(AM_IMP);
        break;
    case 0xf0:
        beq(AM_PC_REL);
        break;
    case 0xfb:
        xce(AM_ACC);
        break;
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
