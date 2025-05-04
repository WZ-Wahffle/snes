#include "cpu_instructions.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

static const char *addressing_mode_strings[] = {
    "Absolute",
    "Absolute Indexed Indirect",
    "Absolute Indexed with X",
    "Absolute Indexed with Y",
    "Absolute Indirect",
    "Absolute Long Indexed With X",
    "Absolute Long",
    "Accumulator",
    "Block Move",
    "Direct Indexed Indirect",
    "Direct Indexed with X",
    "Direct Indexed with Y",
    "Direct Indirect Indexed",
    "Direct Indirect Long Indexed",
    "Direct Indirect Long",
    "Direct Indirect",
    "Direct",
    "Immediate",
    "Implied",
    "Program Counter Relative Long",
    "Program Counter Relative",
    "Stack",
    "Stack Relative",
    "Stack Relative Indirect Indexed",
};

OP(sei) {
    LEGALADDRMODES(AM_IMP);
    set_status_bit(STATUS_IRQOFF, true);
}

OP(cli) {
    LEGALADDRMODES(AM_IMP);
    set_status_bit(STATUS_IRQOFF, false);
}

OP(stz) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_DIR | AM_ZBKX_DIR);

    uint32_t addr = resolve_addr(mode);
    write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), 0);
}

OP(lda) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_ZBKX_DIR | AM_STK_REL | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM);
    uint16_t val = resolve_read16(mode, false, true);
    write_r(R_C, val);
    set_status_bit(STATUS_ZERO, val == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (val & 0x80)
                                        : (val & 0x8000));
}

OP(sta) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L);
    uint32_t addr = resolve_addr(mode);
    uint16_t val = read_r(R_C);
    if (get_status_bit(STATUS_MEMNARROW)) {
        write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    } else {
        write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    }
}

OP(stx) {
    LEGALADDRMODES(AM_ABS | AM_DIR | AM_ZBKY_DIR);
    uint32_t addr = resolve_addr(mode);
    uint16_t val = read_r(R_X);
    if (get_status_bit(STATUS_XNARROW)) {
        write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    } else {
        write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    }
}

OP(sty) {
    LEGALADDRMODES(AM_ABS | AM_DIR | AM_ZBKX_DIR);
    uint32_t addr = resolve_addr(mode);
    uint16_t val = read_r(R_Y);
    if (get_status_bit(STATUS_XNARROW)) {
        write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    } else {
        write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    }
}

OP(clc) {
    LEGALADDRMODES(AM_IMP);
    set_status_bit(STATUS_CARRY, false);
}

OP(xce) {
    LEGALADDRMODES(AM_ACC);
    bool tmp = cpu.emulation_mode;
    cpu.emulation_mode = get_status_bit(STATUS_CARRY);
    set_status_bit(STATUS_CARRY, tmp);
}

OP(xba) {
    LEGALADDRMODES(AM_IMP);
    // register write functions are not used because this works despite
    // emulation flag
    uint8_t lsb = U16_HIBYTE(cpu.c);
    uint8_t msb = U16_LOBYTE(cpu.c);
    cpu.c = TO_U16(lsb, msb);
    set_status_bit(STATUS_ZERO, cpu.c == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (cpu.c & 0x80)
                                        : (cpu.c & 0x8000));
}

OP(rep) {
    LEGALADDRMODES(AM_IMM);
    uint8_t val = resolve_read8(mode);
    cpu.p &= ~val;
}

OP(sep) {
    LEGALADDRMODES(AM_IMM);
    uint8_t val = resolve_read8(mode);
    cpu.p |= val;
}

OP(tcd) {
    LEGALADDRMODES(AM_IMP);
    cpu.d = cpu.c;
    set_status_bit(STATUS_ZERO, cpu.d == 0);
    set_status_bit(STATUS_NEGATIVE, cpu.d & 0x8000);
}

OP(tcs) {
    LEGALADDRMODES(AM_IMP);
    cpu.s = cpu.c;
}

OP(tsb) {
    LEGALADDRMODES(AM_ABS | AM_DIR);
    uint32_t addr = resolve_addr(mode);
    if (get_status_bit(STATUS_MEMNARROW)) {
        uint8_t val = read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
        set_status_bit(STATUS_ZERO, (val & read_r(R_C)) == 0);
        val |= read_r(R_C);
        write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    } else {
        uint16_t val = read_16(U24_LOSHORT(addr), U24_HIBYTE(addr));
        set_status_bit(STATUS_ZERO, (val & read_r(R_C)) == 0);
        val |= read_r(R_C);
        write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    }
}

OP(trb) {
    LEGALADDRMODES(AM_ABS | AM_DIR);
    uint32_t addr = resolve_addr(mode);
    if (get_status_bit(STATUS_MEMNARROW)) {
        uint8_t val = read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
        set_status_bit(STATUS_ZERO, (val & read_r(R_C)) == 0);
        val &= ~read_r(R_C);
        write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    } else {
        uint16_t val = read_16(U24_LOSHORT(addr), U24_HIBYTE(addr));
        set_status_bit(STATUS_ZERO, (val & read_r(R_C)) == 0);
        val &= ~read_r(R_C);
        write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
    }
}

OP(ldx) {
    LEGALADDRMODES(AM_ABS | AM_ABSY | AM_DIR | AM_ZBKY_DIR | AM_IMM);
    uint16_t val = resolve_read16(mode, true, false);
    write_r(R_X, val);
    set_status_bit(STATUS_ZERO, val == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (val & 0x80)
                                        : (val & 0x8000));
}

OP(ldy) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_DIR | AM_ZBKX_DIR | AM_IMM);
    uint16_t val = resolve_read16(mode, true, false);
    write_r(R_Y, val);
    set_status_bit(STATUS_ZERO, val == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (val & 0x80)
                                        : (val & 0x8000));
}

OP(tya) {
    LEGALADDRMODES(AM_ACC);
    write_r(R_C, cpu.y);
    set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (read_r(R_C) & 0x80)
                                        : (read_r(R_C) & 0x8000));
}

OP(tax) {
    LEGALADDRMODES(AM_IMP);
    write_r(R_X, cpu.c);
    set_status_bit(STATUS_ZERO, read_r(R_X) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_X) & 0x80)
                                        : (read_r(R_X) & 0x8000));
}

OP(txa) {
    LEGALADDRMODES(AM_IMP);
    write_r(R_C, cpu.x);
    set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (read_r(R_C) & 0x80)
                                        : (read_r(R_C) & 0x8000));
}

OP(tay) {
    LEGALADDRMODES(AM_IMP);
    write_r(R_Y, cpu.c);
    set_status_bit(STATUS_ZERO, read_r(R_Y) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_Y) & 0x80)
                                        : (read_r(R_Y) & 0x8000));
}

OP(txy) {
    LEGALADDRMODES(AM_IMP);
    write_r(R_Y, cpu.x);
    set_status_bit(STATUS_ZERO, read_r(R_Y) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_Y) & 0x80)
                                        : (read_r(R_Y) & 0x8000));
}

OP(tyx) {
    LEGALADDRMODES(AM_IMP);
    write_r(R_X, cpu.y);
    set_status_bit(STATUS_ZERO, read_r(R_X) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_X) & 0x80)
                                        : (read_r(R_X) & 0x8000));
}

OP(sec) {
    LEGALADDRMODES(AM_IMP);
    set_status_bit(STATUS_CARRY, true);
}

OP(adc) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM);

    uint16_t tmp = resolve_read16(mode, false, true);

    if (get_status_bit(STATUS_MEMNARROW)) {
        uint8_t data = tmp;
        uint16_t result;
        if (!get_status_bit(STATUS_BCD)) {
            result = U16_LOBYTE(cpu.c) + data + get_status_bit(STATUS_CARRY);
        } else {
            result = (U16_LOBYTE(cpu.c) & 0xf) + (data & 0xf) +
                     (get_status_bit(STATUS_CARRY) << 0);
            if (result > 0x9)
                result += 0x6;
            set_status_bit(STATUS_CARRY, result > 0xf);
            result = (U16_LOBYTE(cpu.c) & 0xf0) + (data & 0xf0) +
                     (get_status_bit(STATUS_CARRY) << 4) + (result & 0xf);
        }

        set_status_bit(STATUS_OVERFLOW, ~(U16_LOBYTE(cpu.c) ^ data) &
                                            (U16_LOBYTE(cpu.c) ^ result) &
                                            0x80);
        if (get_status_bit(STATUS_BCD) && result > 0x9f)
            result += 0x60;
        set_status_bit(STATUS_CARRY, result > 0xff);
        set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
        set_status_bit(STATUS_NEGATIVE, result & 0x80);
        write_r(R_C, result);
    } else {
        uint16_t data = tmp;
        uint32_t result;

        if (!get_status_bit(STATUS_BCD)) {
            result = cpu.c + data + get_status_bit(STATUS_CARRY);
        } else {
            result =
                (cpu.c & 0xf) + (data & 0xf) + get_status_bit(STATUS_CARRY);
            if (result > 0x9)
                result += 0x6;
            set_status_bit(STATUS_CARRY, result > 0xf);
            result = (cpu.c & 0xf0) + (data & 0xf0) +
                     (get_status_bit(STATUS_CARRY) << 4) + (result & 0xf);
            if (result > 0x9f)
                result += 0x60;
            set_status_bit(STATUS_CARRY, result > 0xff);
            result = (cpu.c & 0xf00) + (data & 0xf00) +
                     (get_status_bit(STATUS_CARRY) << 8) + (result & 0xff);
            if (result < 0x9ff)
                result += 0x600;
            set_status_bit(STATUS_CARRY, result > 0xfff);
            result = (cpu.c & 0xf000) + (data & 0xf000) +
                     (get_status_bit(STATUS_CARRY) << 12) + (result & 0xfff);
        }

        set_status_bit(STATUS_OVERFLOW,
                       ~(cpu.c ^ data) & (cpu.c ^ result) & 0x8000);
        if (get_status_bit(STATUS_BCD) && result > 0x9fff)
            result += 0x6000;
        set_status_bit(STATUS_CARRY, result > 0xffff);
        set_status_bit(STATUS_ZERO, (result & 0xffff) == 0);
        set_status_bit(STATUS_NEGATIVE, result & 0x8000);
        write_r(R_C, result);
    }
}

// https://github.com/bsnes-emu/bsnes
OP(sbc) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM);
    uint16_t tmp = resolve_read16(mode, false, true);

    if (get_status_bit(STATUS_MEMNARROW)) {
        uint8_t data = tmp;
        data = ~data;
        uint16_t result;
        if (!get_status_bit(STATUS_BCD)) {
            result = U16_LOBYTE(cpu.c) + data + get_status_bit(STATUS_CARRY);
        } else {
            result = (U16_LOBYTE(cpu.c) & 0xf) + (data & 0xf) +
                     (get_status_bit(STATUS_CARRY) << 0);
            if (result < 0x10)
                result -= 0x6;
            set_status_bit(STATUS_CARRY, result > 0xf);
            result = (U16_LOBYTE(cpu.c) & 0xf0) + (data & 0xf0) +
                     (get_status_bit(STATUS_CARRY) << 4) + (result & 0xf);
        }

        set_status_bit(STATUS_OVERFLOW, ~(U16_LOBYTE(cpu.c) ^ data) &
                                            (U16_LOBYTE(cpu.c) ^ result) &
                                            0x80);
        if (get_status_bit(STATUS_BCD) && result < 0x100)
            result -= 0x60;
        set_status_bit(STATUS_CARRY, result > 0xff);
        set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
        set_status_bit(STATUS_NEGATIVE, result & 0x80);
        write_r(R_C, result);
    } else {
        uint16_t data = tmp;
        data = ~data;
        uint32_t result;

        if (!get_status_bit(STATUS_BCD)) {
            result = cpu.c + data + get_status_bit(STATUS_CARRY);
        } else {
            result =
                (cpu.c & 0xf) + (data & 0xf) + get_status_bit(STATUS_CARRY);
            if (result < 0x10)
                result -= 0x6;
            set_status_bit(STATUS_CARRY, result > 0xf);
            result = (cpu.c & 0xf0) + (data & 0xf0) +
                     (get_status_bit(STATUS_CARRY) << 4) + (result & 0xf);
            if (result < 0x100)
                result -= 0x60;
            set_status_bit(STATUS_CARRY, result > 0xff);
            result = (cpu.c & 0xf00) + (data & 0xf00) +
                     (get_status_bit(STATUS_CARRY) << 8) + (result & 0xff);
            if (result < 0x1000)
                result -= 0x600;
            set_status_bit(STATUS_CARRY, result > 0xfff);
            result = (cpu.c & 0xf000) + (data & 0xf000) +
                     (get_status_bit(STATUS_CARRY) << 12) + (result & 0xfff);
        }

        set_status_bit(STATUS_OVERFLOW,
                       ~(cpu.c ^ data) & (cpu.c ^ result) & 0x8000);
        if (get_status_bit(STATUS_BCD) && result < 0x10000)
            result -= 0x6000;
        set_status_bit(STATUS_CARRY, result > 0xffff);
        set_status_bit(STATUS_ZERO, (result & 0xffff) == 0);
        set_status_bit(STATUS_NEGATIVE, result & 0x8000);
        write_r(R_C, result);
    }
}

OP(cmp) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM);
    uint16_t val = resolve_read16(mode, false, true);
    int32_t result = read_r(R_C) - val;
    set_status_bit(STATUS_CARRY, result >= 0);
    set_status_bit(STATUS_ZERO, get_status_bit(STATUS_MEMNARROW)
                                    ? (result & 0xff) == 0
                                    : (result & 0xffff) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (result & 0x80)
                                        : (result & 0x8000));
}

OP(cpx) {
    LEGALADDRMODES(AM_ABS | AM_DIR | AM_IMM);
    uint16_t val = resolve_read16(mode, true, false);
    int32_t result = read_r(R_X) - val;
    set_status_bit(STATUS_CARRY, result >= 0);
    set_status_bit(STATUS_ZERO, get_status_bit(STATUS_XNARROW)
                                    ? (result & 0xff) == 0
                                    : (result & 0xffff) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (result & 0x80)
                                        : (result & 0x8000));
}

OP(cpy) {
    LEGALADDRMODES(AM_ABS | AM_DIR | AM_IMM);
    uint16_t val = resolve_read16(mode, true, false);
    int32_t result = read_r(R_Y) - val;
    set_status_bit(STATUS_CARRY, result >= 0);
    set_status_bit(STATUS_ZERO, get_status_bit(STATUS_XNARROW)
                                    ? (result & 0xff) == 0
                                    : (result & 0xffff) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (result & 0x80)
                                        : (result & 0x8000));
}

OP(inc) {
    LEGALADDRMODES(AM_ABS | AM_ACC | AM_ABSX | AM_DIR | AM_ZBKX_DIR);
    if (mode == AM_ACC) {
        uint16_t val = read_r(R_C) + 1;
        write_r(R_C, val);
        set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
        set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                            ? (read_r(R_C) & 0x80)
                                            : (read_r(R_C) & 0x8000));
    } else {
        uint32_t addr = resolve_addr(mode);
        if (get_status_bit(STATUS_MEMNARROW)) {
            uint8_t val = read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) + 1;
            write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
            set_status_bit(STATUS_ZERO, val == 0);
            set_status_bit(STATUS_NEGATIVE, val & 0x80);
        } else {
            uint16_t val = read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) + 1;
            write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
            set_status_bit(STATUS_ZERO, val == 0);
            set_status_bit(STATUS_NEGATIVE, val & 0x8000);
        }
    }
}

OP(dec) {
    LEGALADDRMODES(AM_ABS | AM_ACC | AM_ABSX | AM_DIR | AM_ZBKX_DIR);
    if (mode == AM_ACC) {
        uint16_t val = read_r(R_C) - 1;
        write_r(R_C, val);
        set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
        set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                            ? (read_r(R_C) & 0x80)
                                            : (read_r(R_C) & 0x8000));
    } else {
        uint32_t addr = resolve_addr(mode);
        if (get_status_bit(STATUS_MEMNARROW)) {
            uint8_t val = read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) - 1;
            write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
            set_status_bit(STATUS_ZERO, val == 0);
            set_status_bit(STATUS_NEGATIVE, val & 0x80);
        } else {
            uint16_t val = read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) - 1;
            write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), val);
            set_status_bit(STATUS_ZERO, val == 0);
            set_status_bit(STATUS_NEGATIVE, val & 0x8000);
        }
    }
}

OP(inx) {
    LEGALADDRMODES(AM_IMP);
    uint16_t val = read_r(R_X);
    val++;
    write_r(R_X, val);
    set_status_bit(STATUS_ZERO, read_r(R_X) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_X) & 0x80)
                                        : (read_r(R_X) & 0x8000));
}

OP(dex) {
    LEGALADDRMODES(AM_IMP);
    uint16_t val = read_r(R_X);
    val--;
    write_r(R_X, val);
    set_status_bit(STATUS_ZERO, read_r(R_X) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_X) & 0x80)
                                        : (read_r(R_X) & 0x8000));
}

OP(iny) {
    LEGALADDRMODES(AM_IMP);
    uint16_t val = read_r(R_Y);
    val++;
    write_r(R_Y, val);
    set_status_bit(STATUS_ZERO, read_r(R_Y) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_Y) & 0x80)
                                        : (read_r(R_Y) & 0x8000));
}

OP(dey) {
    LEGALADDRMODES(AM_IMP);
    uint16_t val = read_r(R_Y);
    val--;
    write_r(R_Y, val);
    set_status_bit(STATUS_ZERO, read_r(R_Y) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_Y) & 0x80)
                                        : (read_r(R_Y) & 0x8000));
}

OP(and_) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM);
    if (get_status_bit(STATUS_MEMNARROW)) {
        write_r(R_C, read_r(R_C) & resolve_read8(mode));
    } else {
        write_r(R_C, read_r(R_C) & resolve_read16(mode, false, false));
    }
    set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (read_r(R_C) & 0x80)
                                        : (read_r(R_C) & 0x8000));
}

OP(ora) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM);
    if (get_status_bit(STATUS_MEMNARROW)) {
        write_r(R_C, read_r(R_C) | resolve_read8(mode));
    } else {
        write_r(R_C, read_r(R_C) | resolve_read16(mode, false, false));
    }
    set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (read_r(R_C) & 0x80)
                                        : (read_r(R_C) & 0x8000));
}

OP(eor) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_ABSY | AM_ABS_L | AM_ABSX_L | AM_DIR |
                   AM_STK_REL | AM_ZBKX_DIR | AM_IND_DIR | AM_IND_DIR_L |
                   AM_STK_REL_INDY | AM_INDX_DIR | AM_INDY_DIR | AM_INDY_DIR_L |
                   AM_IMM | AM_INDX);
    if (get_status_bit(STATUS_MEMNARROW)) {
        write_r(R_C, read_r(R_C) ^ resolve_read8(mode));
    } else {
        write_r(R_C, read_r(R_C) ^ resolve_read16(mode, false, false));
    }
    set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (read_r(R_C) & 0x80)
                                        : (read_r(R_C) & 0x8000));
}

OP(bra) {
    LEGALADDRMODES(AM_PC_REL);
    cpu.pc = resolve_addr(mode);
}

OP(bmi) {
    LEGALADDRMODES(AM_PC_REL);
    if (get_status_bit(STATUS_NEGATIVE)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(bpl) {
    LEGALADDRMODES(AM_PC_REL);
    if (!get_status_bit(STATUS_NEGATIVE)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(beq) {
    LEGALADDRMODES(AM_PC_REL);
    if (get_status_bit(STATUS_ZERO)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(bne) {
    LEGALADDRMODES(AM_PC_REL);
    if (!get_status_bit(STATUS_ZERO)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(bcs) {
    LEGALADDRMODES(AM_PC_REL);
    if (get_status_bit(STATUS_CARRY)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(bcc) {
    LEGALADDRMODES(AM_PC_REL);
    if (!get_status_bit(STATUS_CARRY)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(bvs) {
    LEGALADDRMODES(AM_PC_REL);
    if (get_status_bit(STATUS_OVERFLOW)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(bvc) {
    LEGALADDRMODES(AM_PC_REL);
    if (!get_status_bit(STATUS_OVERFLOW)) {
        cpu.pc = resolve_addr(mode);
    } else {
        cpu.pc++;
    }
}

OP(jmp) {
    LEGALADDRMODES(AM_ABS | AM_IND | AM_INDX);
    uint32_t addr = resolve_addr(mode);
    if (mode == AM_ABS) {
        cpu.pc = addr;
    } else if (mode == AM_IND) {
        cpu.pc = read_16(U24_LOSHORT(addr), 0);
    } else {
        cpu.pc = read_16(U24_LOSHORT(addr), cpu.pbr);
    }
}

OP(jml) {
    LEGALADDRMODES(AM_IND | AM_ABS_L);
    uint32_t addr = resolve_addr(mode);
    if (mode == AM_IND) {
        uint32_t target = read_24(U24_LOSHORT(addr), U24_HIBYTE(addr));
        cpu.pc = U24_LOSHORT(target);
        cpu.pbr = U24_HIBYTE(target);
    } else {
        cpu.pc = U24_LOSHORT(addr);
        cpu.pbr = U24_HIBYTE(addr);
    }
}

OP(jsr) {
    LEGALADDRMODES(AM_ABS | AM_INDX);
    uint16_t addr = resolve_addr(mode);
    push_16(cpu.pc - 1);
    cpu.pc = addr;
}

OP(jsl) {
    LEGALADDRMODES(AM_ABS_L);
    uint32_t addr = resolve_addr(mode);
    push_8(cpu.pbr);
    push_16(cpu.pc - 1);
    cpu.pc = U24_LOSHORT(addr);
    cpu.pbr = U24_HIBYTE(addr);
}

OP(rts) {
    LEGALADDRMODES(AM_IMP);
    cpu.pc = pop_16() + 1;
}

OP(rtl) {
    LEGALADDRMODES(AM_IMP);
    uint32_t addr = pop_24() + 1;
    cpu.pbr = U24_HIBYTE(addr);
    cpu.pc = U24_LOSHORT(addr);
}

OP(rti) {
    LEGALADDRMODES(AM_STK);
    cpu.p = pop_8();
    cpu.pc = pop_16();
    if (!cpu.emulation_mode)
        cpu.pbr = pop_8();
}

OP(php) {
    LEGALADDRMODES(AM_STK);
    push_8(cpu.p);
}

OP(plp) {
    LEGALADDRMODES(AM_STK);
    cpu.p = pop_8();
}

OP(pha) {
    LEGALADDRMODES(AM_STK);
    if (get_status_bit(STATUS_MEMNARROW)) {
        push_8(read_r(R_C));
    } else {
        push_16(read_r(R_C));
    }
}

OP(pla) {
    LEGALADDRMODES(AM_STK);
    if (get_status_bit(STATUS_MEMNARROW)) {
        write_r(R_C, pop_8());
    } else {
        write_r(R_C, pop_16());
    }
    set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (read_r(R_C) & 0x80)
                                        : read_r(R_C) & 0x8000);
}

OP(phx) {
    LEGALADDRMODES(AM_STK);
    if (get_status_bit(STATUS_XNARROW)) {
        push_8(read_r(R_X));
    } else {
        push_16(read_r(R_X));
    }
}

OP(plx) {
    LEGALADDRMODES(AM_STK);
    if (get_status_bit(STATUS_XNARROW)) {
        write_r(R_X, pop_8());
    } else {
        write_r(R_X, pop_16());
    }
    set_status_bit(STATUS_ZERO, read_r(R_X) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_X) & 0x80)
                                        : read_r(R_X) & 0x8000);
}

OP(phy) {
    LEGALADDRMODES(AM_STK);
    if (get_status_bit(STATUS_XNARROW)) {
        push_8(read_r(R_Y));
    } else {
        push_16(read_r(R_Y));
    }
}

OP(ply) {
    LEGALADDRMODES(AM_STK);
    if (get_status_bit(STATUS_XNARROW)) {
        write_r(R_Y, pop_8());
    } else {
        write_r(R_Y, pop_16());
    }
    set_status_bit(STATUS_ZERO, read_r(R_Y) == 0);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_XNARROW)
                                        ? (read_r(R_Y) & 0x80)
                                        : read_r(R_Y) & 0x8000);
}

OP(phb) {
    LEGALADDRMODES(AM_STK);
    push_8(cpu.dbr);
}

OP(plb) {
    LEGALADDRMODES(AM_STK);
    cpu.dbr = pop_8();
    set_status_bit(STATUS_ZERO, cpu.dbr == 0);
    set_status_bit(STATUS_NEGATIVE, cpu.dbr & 0x80);
}

OP(phk) {
    LEGALADDRMODES(AM_STK);
    push_8(cpu.pbr);
}

OP(rol) {
    LEGALADDRMODES(AM_ABS | AM_ACC | AM_ABSX | AM_DIR | AM_ZBKX_DIR);
    if (mode == AM_ACC) {
        bool carry = get_status_bit(STATUS_CARRY);
        set_status_bit(STATUS_CARRY, get_status_bit(STATUS_MEMNARROW)
                                         ? (read_r(R_C) & 0x80)
                                         : read_r(R_C) & 0x8000);
        write_r(R_C, (read_r(R_C) << 1) | carry);
        set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
        set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                            ? (read_r(R_C) & 0x80)
                                            : read_r(R_C) & 0x8000);
    } else {
        bool carry = get_status_bit(STATUS_CARRY);
        uint32_t addr = resolve_addr(mode);
        if (get_status_bit(STATUS_MEMNARROW)) {
            set_status_bit(STATUS_CARRY,
                           read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) & 0x80);
            uint8_t result =
                (read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) << 1) | carry;
            write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x80);
        } else {
            set_status_bit(STATUS_CARRY,
                           read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) &
                               0x8000);
            uint16_t result =
                (read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) << 1) | carry;
            write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x8000);
        }
    }
}

OP(ror) {
    LEGALADDRMODES(AM_ABS | AM_ACC | AM_ABSX | AM_DIR | AM_ZBKX_DIR);
    if (mode == AM_ACC) {
        bool carry = get_status_bit(STATUS_CARRY);
        set_status_bit(STATUS_CARRY, read_r(R_C) & 1);
        write_r(R_C, (read_r(R_C) >> 1) | (carry << 7));
        set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
        set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                            ? (read_r(R_C) & 0x80)
                                            : read_r(R_C) & 0x8000);
    } else {
        bool carry = get_status_bit(STATUS_CARRY);
        uint32_t addr = resolve_addr(mode);
        if (get_status_bit(STATUS_MEMNARROW)) {
            set_status_bit(STATUS_CARRY,
                           read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) & 1);
            uint8_t result =
                (read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) >> 1) |
                (carry << 7);
            write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x80);
        } else {
            set_status_bit(STATUS_CARRY,
                           read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) & 1);
            uint16_t result =
                (read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) >> 1) |
                (carry << 7);
            write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x8000);
        }
    }
}

OP(asl) {
    LEGALADDRMODES(AM_ABS | AM_ACC | AM_ABSX | AM_DIR | AM_ZBKX_DIR);
    if (mode == AM_ACC) {
        set_status_bit(STATUS_CARRY, get_status_bit(STATUS_MEMNARROW)
                                         ? (read_r(R_C) & 0x80)
                                         : read_r(R_C) & 0x8000);
        write_r(R_C, read_r(R_C) << 1);
        set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
        set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                            ? (read_r(R_C) & 0x80)
                                            : read_r(R_C) & 0x8000);
    } else {
        uint32_t addr = resolve_addr(mode);
        if (get_status_bit(STATUS_MEMNARROW)) {
            set_status_bit(STATUS_CARRY,
                           read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) & 0x80);
            uint8_t result = read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) << 1;
            write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x80);
        } else {
            set_status_bit(STATUS_CARRY,
                           read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) &
                               0x8000);
            uint16_t result =
                (read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) << 1);
            write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x8000);
        }
    }
}

OP(lsr) {
    LEGALADDRMODES(AM_ABS | AM_ACC | AM_ABSX | AM_DIR | AM_ZBKX_DIR);
    if (mode == AM_ACC) {
        set_status_bit(STATUS_CARRY, read_r(R_C) & 1);
        write_r(R_C, read_r(R_C) >> 1);
        set_status_bit(STATUS_ZERO, read_r(R_C) == 0);
        set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                            ? (read_r(R_C) & 0x80)
                                            : read_r(R_C) & 0x8000);
    } else {
        uint32_t addr = resolve_addr(mode);
        if (get_status_bit(STATUS_MEMNARROW)) {
            set_status_bit(STATUS_CARRY,
                           read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) & 1);
            uint8_t result = read_8(U24_LOSHORT(addr), U24_HIBYTE(addr)) >> 1;
            write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x80);
        } else {
            set_status_bit(STATUS_CARRY,
                           read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) & 1);
            uint16_t result =
                (read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)) >> 1);
            write_16(U24_LOSHORT(addr), U24_HIBYTE(addr), result);
            set_status_bit(STATUS_ZERO, result == 0);
            set_status_bit(STATUS_NEGATIVE, result & 0x8000);
        }
    }
}

OP(bit) {
    LEGALADDRMODES(AM_IMM | AM_ABS | AM_DIR | AM_ABSX | AM_ZBKX_DIR);
    uint16_t operand = resolve_read16(mode, false, true);
    set_status_bit(STATUS_NEGATIVE, get_status_bit(STATUS_MEMNARROW)
                                        ? (operand & 0x80)
                                        : (operand & 0x8000));
    set_status_bit(STATUS_OVERFLOW, get_status_bit(STATUS_MEMNARROW)
                                        ? (operand & 0x40)
                                        : (operand & 0x4000));
    set_status_bit(STATUS_ZERO, (operand & read_r(R_C)) == 0);
}

OP(mvn) {
    LEGALADDRMODES(AM_BLK);
    uint8_t dest_b = next_8();
    uint8_t src_b = next_8();

    while(cpu.c != 0xffff) {
        write_8(read_r(R_Y), dest_b, read_8(read_r(R_X), src_b));
        write_r(R_X, read_r(R_X) + 1);
        write_r(R_Y, read_r(R_Y) + 1);
        cpu.c--;
    }

    cpu.dbr = dest_b;
}

OP(per) {
    LEGALADDRMODES(AM_PC_REL_L);
    uint16_t operand = next_16();
    push_16(cpu.pc + operand);
}
