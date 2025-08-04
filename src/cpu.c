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
        if (cpu.emulation_mode)
            cpu.s = TO_U16(U16_LOBYTE(cpu.s), 1);
        return cpu.s;
    case R_D:
        return cpu.d;
        break;
    default:
        UNREACHABLE_SWITCH(reg);
    }
}

uint8_t read_8(uint16_t addr, uint8_t bank) {
    for (uint32_t i = 0; i < cpu.breakpoints_size; i++) {
        if (cpu.breakpoints[i].valid && cpu.breakpoints[i].read &&
            TO_U24(addr, bank) == cpu.breakpoints[i].line) {
            cpu.state = STATE_STOPPED;
            break;
        }
    }
    return mmu_read(addr, bank, true);
}

uint8_t read_8_no_log(uint16_t addr, uint8_t bank) {
    return mmu_read(addr, bank, false);
}

uint16_t read_16(uint16_t addr, uint8_t bank) {
    return TO_U16(read_8(addr, bank),
                  read_8(addr + 1, bank + (addr == 0xffff)));
}

uint32_t read_24(uint16_t addr, uint8_t bank) {
    return TO_U24(read_16(addr, bank),
                  read_8(addr + 2, bank + (addr > 0xfffd)));
}

uint16_t read_16_dir(uint16_t addr, bool hack_flag, bool new_instruction) {
    if (!new_instruction && hack_flag && cpu.emulation_mode &&
        cpu.d % 0x100 != 0) {
        return TO_U16(read_8(addr + cpu.d, 0),
                      read_8(TO_U16(U16_LOBYTE(addr + cpu.d + 1),
                                    U16_HIBYTE(addr + cpu.d)),
                             0));
    }
    if (cpu.emulation_mode && cpu.d % 0x100 == 0) {
        uint16_t t_lo = TO_U16(U16_LOBYTE(cpu.d + addr), U16_HIBYTE(cpu.d));
        uint16_t t_hi = TO_U16(U16_LOBYTE(cpu.d + addr + 1), U16_HIBYTE(cpu.d));
        return TO_U16(read_8(t_lo, 0), read_8(t_hi, 0));
    } else {
        return read_16(addr + cpu.d, 0);
    }
}

uint32_t read_24_dir(uint16_t addr) { return read_24(addr + cpu.d, 0); }

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
        if (cpu.emulation_mode)
            cpu.s = TO_U16(U16_LOBYTE(cpu.s), 1);
        break;
    case R_D:
        cpu.d = val;
        break;
    }
}

void write_8(uint16_t addr, uint8_t bank, uint8_t val) {
    for (uint32_t i = 0; i < cpu.breakpoints_size; i++) {
        if (cpu.breakpoints[i].valid && cpu.breakpoints[i].write &&
            TO_U24(addr, bank) == cpu.breakpoints[i].line) {
            cpu.state = STATE_STOPPED;
            break;
        }
    }
    mmu_write(addr, bank, val, true);
}

void write_16(uint16_t addr, uint8_t bank, uint16_t val) {
    write_8(addr, bank, U16_LOBYTE(val));
    write_8(addr + 1, bank + (addr == 0xffff), U16_HIBYTE(val));
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

void push_8(uint8_t val) {
    write_8(read_r(R_S), 0, val);
    cpu.s--;
}
void push_16(uint16_t val) {
    push_8(U16_HIBYTE(val));
    push_8(U16_LOBYTE(val));
}
void push_24(uint32_t val) {
    push_8(U24_HIBYTE(val));
    push_16(U24_LOSHORT(val));
}
uint8_t pop_8(void) {
    cpu.s++;
    return read_8(read_r(R_S), 0);
}
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
        ret = read_16(next_16() + read_r(R_X), cpu.pbr);
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
        ret = TO_U24(read_16_dir(next_8() + read_r(R_X), true, false), cpu.dbr);
        break;
    case AM_ZBKX_DIR: // needs dir
        ret = TO_U24(next_8() + read_r(R_X), 0);
        break;
    case AM_ZBKY_DIR: // needs dir
        ret = TO_U24(next_8() + read_r(R_Y), 0);
        break;
    case AM_INDY_DIR:
        ret =
            TO_U24(read_16_dir(next_8(), false, false), cpu.dbr) + read_r(R_Y);
        break;
    case AM_INDY_DIR_L:
        ret = read_24_dir(next_8()) + read_r(R_Y);
        break;
    case AM_IND_DIR_L:
        ret = read_24_dir(next_8());
        break;
    case AM_IND_DIR:
        ret = TO_U24(read_16_dir(next_8(), false, false), cpu.dbr);
        break;
    case AM_DIR: // needs dir
        ret = next_8();
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
        if ((get_status_bit(STATUS_XNARROW) && respect_x) ||
            (get_status_bit(STATUS_MEMNARROW) && respect_m)) {
            if (mode & (AM_ZBKX_DIR | AM_ZBKY_DIR | AM_DIR)) {
                if (cpu.emulation_mode && cpu.d % 256 == 0) {
                    return read_8(
                        TO_U16(U16_LOBYTE(addr + cpu.d), U16_HIBYTE(cpu.d)), 0);
                }
                return read_8(U24_LOSHORT(addr + cpu.d), 0);
            }
            return read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
        }

        if (mode & (AM_ZBKX_DIR | AM_ZBKY_DIR | AM_DIR)) {
            return read_16_dir(U24_LOSHORT(addr), false, false);
        }
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
    case AM_ABS:
    case AM_ABSX:
    case AM_ABSY:
    case AM_ABS_L:
    case AM_ABSX_L:
    case AM_DIR:
    case AM_IND_DIR:
    case AM_ZBKX_DIR:
    case AM_INDX_DIR:
    case AM_INDY_DIR:
    case AM_INDY_DIR_L:
    case AM_STK_REL:
    case AM_IND_DIR_L:
    case AM_STK_REL_INDY: {
        uint32_t addr = resolve_addr(mode);
        if (mode & (AM_ZBKX_DIR | AM_ZBKY_DIR | AM_DIR)) {
            if (cpu.emulation_mode && cpu.d % 256 == 0) {
                return read_8(
                    TO_U16(U16_LOBYTE(addr + cpu.d), U16_HIBYTE(cpu.d)), 0);
            }
            return read_8(U24_LOSHORT(cpu.d + addr), 0);
        }
        return read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
    }
    default:
        UNREACHABLE_SWITCH(mode);
    }
}

void cpu_reset(void) {
    cpu.speed = 1;
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
    if (cpu.waiting) {
        // WAI opcode, CPU is in low power mode while waiting for interrupts
        cpu.remaining_clocks = 0;
        return;
    }
    uint8_t opcode = next_8();
    log_message(LOG_LEVEL_VERBOSE, "CPU fetched opcode 0x%02x", opcode);
    cpu.opcode_history[cpu.history_idx] = opcode;
    cpu.pc_history[cpu.history_idx] = TO_U24(cpu.pc, cpu.pbr);
    cpu.history_idx++;
    // TODO: disambiguate cpu cycles taking 6, 8 or 12 clock cycles

    // remaining clocks increments at 341 * 262 * 60 * 4 = 21.44MHz.
    // The highest achievable CPU clock speed is roughly 3.58 MHz,
    // which corresponds to 6 master clocks per CPU clock, hence the
    // factor of 6
    cpu.remaining_clocks -= 6 * cpu_cycle_counts[opcode];
    switch (opcode) {
    case 0x00:
        brk(AM_IMP);
        break;
    case 0x01:
        ora(AM_INDX_DIR);
        break;
    case 0x02:
        cop(AM_IMP);
        break;
    case 0x03:
        ora(AM_STK_REL);
        break;
    case 0x04:
        tsb(AM_DIR);
        break;
    case 0x05:
        ora(AM_DIR);
        break;
    case 0x06:
        asl(AM_DIR);
        break;
    case 0x07:
        ora(AM_IND_DIR_L);
        break;
    case 0x08:
        php(AM_STK);
        break;
    case 0x09:
        ora(AM_IMM);
        break;
    case 0x0a:
        asl(AM_ACC);
        break;
    case 0x0b:
        phd(AM_STK);
        break;
    case 0x0c:
        tsb(AM_ABS);
        break;
    case 0x0d:
        ora(AM_ABS);
        break;
    case 0x0e:
        asl(AM_ABS);
        break;
    case 0x0f:
        ora(AM_ABS_L);
        break;
    case 0x10:
        bpl(AM_PC_REL);
        break;
    case 0x11:
        ora(AM_INDY_DIR);
        break;
    case 0x12:
        ora(AM_IND_DIR);
        break;
    case 0x13:
        ora(AM_STK_REL_INDY);
        break;
    case 0x14:
        trb(AM_DIR);
        break;
    case 0x15:
        ora(AM_ZBKX_DIR);
        break;
    case 0x16:
        asl(AM_ZBKX_DIR);
        break;
    case 0x17:
        ora(AM_INDY_DIR_L);
        break;
    case 0x18:
        clc(AM_IMP);
        break;
    case 0x19:
        ora(AM_ABSY);
        break;
    case 0x1a:
        inc(AM_ACC);
        break;
    case 0x1b:
        tcs(AM_IMP);
        break;
    case 0x1c:
        trb(AM_ABS);
        break;
    case 0x1d:
        ora(AM_ABSX);
        break;
    case 0x1e:
        asl(AM_ABSX);
        break;
    case 0x1f:
        ora(AM_ABSX_L);
        break;
    case 0x20:
        jsr(AM_ABS);
        break;
    case 0x21:
        and_(AM_INDX_DIR);
        break;
    case 0x22:
        jsl(AM_ABS_L);
        break;
    case 0x23:
        and_(AM_STK_REL);
        break;
    case 0x24:
        bit(AM_DIR);
        break;
    case 0x25:
        and_(AM_DIR);
        break;
    case 0x26:
        rol(AM_DIR);
        break;
    case 0x27:
        and_(AM_IND_DIR_L);
        break;
    case 0x28:
        plp(AM_STK);
        break;
    case 0x29:
        and_(AM_IMM);
        break;
    case 0x2a:
        rol(AM_ACC);
        break;
    case 0x2b:
        pld(AM_STK);
        break;
    case 0x2c:
        bit(AM_ABS);
        break;
    case 0x2d:
        and_(AM_ABS);
        break;
    case 0x2e:
        rol(AM_ABS);
        break;
    case 0x2f:
        and_(AM_ABS_L);
        break;
    case 0x30:
        bmi(AM_PC_REL);
        break;
    case 0x31:
        and_(AM_INDY_DIR);
        break;
    case 0x32:
        and_(AM_IND_DIR);
        break;
    case 0x33:
        and_(AM_STK_REL_INDY);
        break;
    case 0x34:
        bit(AM_ZBKX_DIR);
        break;
    case 0x35:
        and_(AM_ZBKX_DIR);
        break;
    case 0x36:
        rol(AM_ZBKX_DIR);
        break;
    case 0x37:
        and_(AM_INDY_DIR_L);
        break;
    case 0x38:
        sec(AM_IMP);
        break;
    case 0x39:
        and_(AM_ABSY);
        break;
    case 0x3a:
        dec(AM_ACC);
        break;
    case 0x3b:
        tsc(AM_IMP);
        break;
    case 0x3c:
        bit(AM_ABSX);
        break;
    case 0x3d:
        and_(AM_ABSX);
        break;
    case 0x3e:
        rol(AM_ABSX);
        break;
    case 0x3f:
        and_(AM_ABSX_L);
        break;
    case 0x40:
        rti(AM_STK);
        break;
    case 0x41:
        eor(AM_INDX_DIR);
        break;
    case 0x42:
        // this page intentionally left blank
        (void)next_8();
        break;
    case 0x43:
        eor(AM_STK_REL);
        break;
    case 0x44:
        mvp(AM_BLK);
        break;
    case 0x45:
        eor(AM_DIR);
        break;
    case 0x46:
        lsr(AM_DIR);
        break;
    case 0x47:
        eor(AM_IND_DIR_L);
        break;
    case 0x48:
        pha(AM_STK);
        break;
    case 0x49:
        eor(AM_IMM);
        break;
    case 0x4a:
        lsr(AM_ACC);
        break;
    case 0x4b:
        phk(AM_STK);
        break;
    case 0x4c:
        jmp(AM_ABS);
        break;
    case 0x4d:
        eor(AM_ABS);
        break;
    case 0x4e:
        lsr(AM_ABS);
        break;
    case 0x4f:
        eor(AM_ABS_L);
        break;
    case 0x50:
        bvc(AM_PC_REL);
        break;
    case 0x51:
        eor(AM_INDY_DIR);
        break;
    case 0x52:
        eor(AM_IND_DIR);
        break;
    case 0x53:
        eor(AM_STK_REL_INDY);
        break;
    case 0x54:
        mvn(AM_BLK);
        break;
    case 0x55:
        eor(AM_ZBKX_DIR);
        break;
    case 0x56:
        lsr(AM_ZBKX_DIR);
        break;
    case 0x57:
        eor(AM_INDY_DIR_L);
        break;
    case 0x58:
        cli(AM_IMP);
        break;
    case 0x59:
        eor(AM_ABSY);
        break;
    case 0x5a:
        phy(AM_STK);
        break;
    case 0x5b:
        tcd(AM_IMP);
        break;
    case 0x5c:
        jml(AM_ABS_L);
        break;
    case 0x5d:
        eor(AM_ABSX);
        break;
    case 0x5e:
        lsr(AM_ABSX);
        break;
    case 0x5f:
        eor(AM_ABSX_L);
        break;
    case 0x60:
        rts(AM_IMP);
        break;
    case 0x61:
        adc(AM_INDX_DIR);
        break;
    case 0x62:
        per(AM_PC_REL_L);
        break;
    case 0x63:
        adc(AM_STK_REL);
        break;
    case 0x64:
        stz(AM_DIR);
        break;
    case 0x65:
        adc(AM_DIR);
        break;
    case 0x66:
        ror(AM_DIR);
        break;
    case 0x67:
        adc(AM_IND_DIR_L);
        break;
    case 0x68:
        pla(AM_STK);
        break;
    case 0x69:
        adc(AM_IMM);
        break;
    case 0x6a:
        ror(AM_ACC);
        break;
    case 0x6b:
        rtl(AM_IMP);
        break;
    case 0x6c:
        jmp(AM_IND);
        break;
    case 0x6d:
        adc(AM_ABS);
        break;
    case 0x6e:
        ror(AM_ABS);
        break;
    case 0x6f:
        adc(AM_ABS_L);
        break;
    case 0x70:
        bvs(AM_PC_REL);
        break;
    case 0x71:
        adc(AM_INDY_DIR);
        break;
    case 0x72:
        adc(AM_IND_DIR);
        break;
    case 0x73:
        adc(AM_STK_REL_INDY);
        break;
    case 0x74:
        stz(AM_ZBKX_DIR);
        break;
    case 0x75:
        adc(AM_ZBKX_DIR);
        break;
    case 0x76:
        ror(AM_ZBKX_DIR);
        break;
    case 0x77:
        adc(AM_INDY_DIR_L);
        break;
    case 0x78:
        sei(AM_IMP);
        break;
    case 0x79:
        adc(AM_ABSY);
        break;
    case 0x7a:
        ply(AM_STK);
        break;
    case 0x7b:
        tdc(AM_IMP);
        break;
    case 0x7c:
        jmp(AM_INDX);
        break;
    case 0x7d:
        adc(AM_ABSX);
        break;
    case 0x7e:
        ror(AM_ABSX);
        break;
    case 0x7f:
        adc(AM_ABSX_L);
        break;
    case 0x80:
        bra(AM_PC_REL);
        break;
    case 0x81:
        sta(AM_INDX_DIR);
        break;
    case 0x82:
        brl(AM_PC_REL_L);
        break;
    case 0x83:
        sta(AM_STK_REL);
        break;
    case 0x84:
        sty(AM_DIR);
        break;
    case 0x85:
        sta(AM_DIR);
        break;
    case 0x86:
        stx(AM_DIR);
        break;
    case 0x87:
        sta(AM_IND_DIR_L);
        break;
    case 0x88:
        dey(AM_IMP);
        break;
    case 0x89:
        bit(AM_IMM);
        break;
    case 0x8a:
        txa(AM_IMP);
        break;
    case 0x8b:
        phb(AM_STK);
        break;
    case 0x8c:
        sty(AM_ABS);
        break;
    case 0x8d:
        sta(AM_ABS);
        break;
    case 0x8e:
        stx(AM_ABS);
        break;
    case 0x8f:
        sta(AM_ABS_L);
        break;
    case 0x90:
        bcc(AM_PC_REL);
        break;
    case 0x91:
        sta(AM_INDY_DIR);
        break;
    case 0x92:
        sta(AM_IND_DIR);
        break;
    case 0x93:
        sta(AM_STK_REL_INDY);
        break;
    case 0x94:
        sty(AM_ZBKX_DIR);
        break;
    case 0x95:
        sta(AM_ZBKX_DIR);
        break;
    case 0x96:
        stx(AM_ZBKY_DIR);
        break;
    case 0x97:
        sta(AM_INDY_DIR_L);
        break;
    case 0x98:
        tya(AM_ACC);
        break;
    case 0x99:
        sta(AM_ABSY);
        break;
    case 0x9a:
        txs(AM_IMP);
        break;
    case 0x9b:
        txy(AM_IMP);
        break;
    case 0x9c:
        stz(AM_ABS);
        break;
    case 0x9d:
        sta(AM_ABSX);
        break;
    case 0x9e:
        stz(AM_ABSX);
        break;
    case 0x9f:
        sta(AM_ABSX_L);
        break;
    case 0xa0:
        ldy(AM_IMM);
        break;
    case 0xa1:
        lda(AM_INDX_DIR);
        break;
    case 0xa2:
        ldx(AM_IMM);
        break;
    case 0xa3:
        lda(AM_STK_REL);
        break;
    case 0xa4:
        ldy(AM_DIR);
        break;
    case 0xa5:
        lda(AM_DIR);
        break;
    case 0xa6:
        ldx(AM_DIR);
        break;
    case 0xa7:
        lda(AM_IND_DIR_L);
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
    case 0xab:
        plb(AM_STK);
        break;
    case 0xac:
        ldy(AM_ABS);
        break;
    case 0xad:
        lda(AM_ABS);
        break;
    case 0xae:
        ldx(AM_ABS);
        break;
    case 0xaf:
        lda(AM_ABS_L);
        break;
    case 0xb0:
        bcs(AM_PC_REL);
        break;
    case 0xb1:
        lda(AM_INDY_DIR);
        break;
    case 0xb2:
        lda(AM_IND_DIR);
        break;
    case 0xb3:
        lda(AM_STK_REL_INDY);
        break;
    case 0xb4:
        ldy(AM_ZBKX_DIR);
        break;
    case 0xb5:
        lda(AM_ZBKX_DIR);
        break;
    case 0xb6:
        ldx(AM_ZBKY_DIR);
        break;
    case 0xb7:
        lda(AM_INDY_DIR_L);
        break;
    case 0xb8:
        clv(AM_IMP);
        break;
    case 0xb9:
        lda(AM_ABSY);
        break;
    case 0xba:
        tsx(AM_IMP);
        break;
    case 0xbb:
        tyx(AM_IMP);
        break;
    case 0xbc:
        ldy(AM_ABSX);
        break;
    case 0xbd:
        lda(AM_ABSX);
        break;
    case 0xbe:
        ldx(AM_ABSY);
        break;
    case 0xbf:
        lda(AM_ABSX_L);
        break;
    case 0xc0:
        cpy(AM_IMM);
        break;
    case 0xc1:
        cmp(AM_INDX_DIR);
        break;
    case 0xc2:
        rep(AM_IMM);
        break;
    case 0xc3:
        cmp(AM_STK_REL);
        break;
    case 0xc4:
        cpy(AM_DIR);
        break;
    case 0xc5:
        cmp(AM_DIR);
        break;
    case 0xc6:
        dec(AM_DIR);
        break;
    case 0xc7:
        cmp(AM_IND_DIR_L);
        break;
    case 0xc8:
        iny(AM_IMP);
        break;
    case 0xc9:
        cmp(AM_IMM);
        break;
    case 0xca:
        dex(AM_IMP);
        break;
    case 0xcb:
        wai(AM_IMP);
        break;
    case 0xcc:
        cpy(AM_ABS);
        break;
    case 0xcd:
        cmp(AM_ABS);
        break;
    case 0xce:
        dec(AM_ABS);
        break;
    case 0xcf:
        cmp(AM_ABS_L);
        break;
    case 0xd0:
        bne(AM_PC_REL);
        break;
    case 0xd1:
        cmp(AM_INDY_DIR);
        break;
    case 0xd2:
        cmp(AM_IND_DIR);
        break;
    case 0xd3:
        cmp(AM_STK_REL_INDY);
        break;
    case 0xd4:
        pei(AM_STK);
        break;
    case 0xd5:
        cmp(AM_ZBKX_DIR);
        break;
    case 0xd6:
        dec(AM_ZBKX_DIR);
        break;
    case 0xd7:
        cmp(AM_INDY_DIR_L);
        break;
    case 0xd8:
        cld(AM_IMP);
        break;
    case 0xd9:
        cmp(AM_ABSY);
        break;
    case 0xda:
        phx(AM_STK);
        break;
    case 0xdc:
        jml(AM_IND);
        break;
    case 0xdd:
        cmp(AM_ABSX);
        break;
    case 0xde:
        dec(AM_ABSX);
        break;
    case 0xdf:
        cmp(AM_ABSX_L);
        break;
    case 0xe0:
        cpx(AM_IMM);
        break;
    case 0xe1:
        sbc(AM_INDX_DIR);
        break;
    case 0xe2:
        sep(AM_IMM);
        break;
    case 0xe3:
        sbc(AM_STK_REL);
        break;
    case 0xe4:
        cpx(AM_DIR);
        break;
    case 0xe5:
        sbc(AM_DIR);
        break;
    case 0xe6:
        inc(AM_DIR);
        break;
    case 0xe7:
        sbc(AM_IND_DIR_L);
        break;
    case 0xe8:
        inx(AM_IMP);
        break;
    case 0xe9:
        sbc(AM_IMM);
        break;
    case 0xea:
        // this page intentionally left blank
        break;
    case 0xeb:
        xba(AM_IMP);
        break;
    case 0xec:
        cpx(AM_ABS);
        break;
    case 0xed:
        sbc(AM_ABS);
        break;
    case 0xee:
        inc(AM_ABS);
        break;
    case 0xef:
        sbc(AM_ABS_L);
        break;
    case 0xf0:
        beq(AM_PC_REL);
        break;
    case 0xf1:
        sbc(AM_INDY_DIR);
        break;
    case 0xf2:
        sbc(AM_IND_DIR);
        break;
    case 0xf3:
        sbc(AM_STK_REL_INDY);
        break;
    case 0xf4:
        pea(AM_STK);
        break;
    case 0xf5:
        sbc(AM_ZBKX_DIR);
        break;
    case 0xf6:
        inc(AM_ZBKX_DIR);
        break;
    case 0xf7:
        sbc(AM_INDY_DIR_L);
        break;
    case 0xf8:
        sed(AM_IMP);
        break;
    case 0xf9:
        sbc(AM_ABSY);
        break;
    case 0xfa:
        plx(AM_STK);
        break;
    case 0xfb:
        xce(AM_ACC);
        break;
    case 0xfc:
        jsr(AM_INDX);
        break;
    case 0xfd:
        sbc(AM_ABSX);
        break;
    case 0xfe:
        inc(AM_ABSX);
        break;
    case 0xff:
        sbc(AM_ABSX_L);
        break;
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
