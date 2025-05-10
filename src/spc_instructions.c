#include "spc_instructions.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

static const char *addressing_mode_strings[] = {
    "Direct Page - d",
    "X-Indexed Direct Page - d,X",
    "Y-Indexed Direct Page - d,Y",
    "Indirect - (X)",
    "Indirect Auto-Increment - (X)+",
    "Direct Page to Direct Page - dd,ds",
    "Indirect Page to Indirect Page - (X),(Y)",
    "Immediate Data to Direct Page - d,#",
    "Direct Page Bit - d.b",
    "Direct Page Bit Relative - d.b,r",
    "Absolute Boolean Bit - m.b",
    "Absolute - a",
    "Absolute X-Indexed Indirect - (a,X)",
    "X-Indexed Absolute - a,X",
    "Y-Indexed Absolute - a,Y",
    "X-Indexed Indirect - (d,X)",
    "Indirect Y-Indexed - (d),Y",
    "Relative - r",
    "Immediate - #",
    "Accumulator - A",
    "Implied",
};

OP(lda) {
    LEGALADDRMODES(SM_IMM | SM_ABS | SM_ABSY | SM_ABSX | SM_DIR_PAGE |
                   SM_DIR_PAGEX | SM_INDIRECT | SM_INDIRECT_INC | SM_INDX |
                   SM_INDY);
    spc.a = spc_resolve_read(mode);
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}

OP(ldx) {
    LEGALADDRMODES(SM_IMM | SM_DIR_PAGE | SM_DIR_PAGEY | SM_ABS);
    spc.x = spc_resolve_read(mode);
    spc_set_status_bit(STATUS_ZERO, spc.x == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.x & 0x80);
}

OP(ldy) {
    LEGALADDRMODES(SM_IMM | SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS);
    spc.y = spc_resolve_read(mode);
    spc_set_status_bit(STATUS_ZERO, spc.y == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.y & 0x80);
}

OP(ldw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t addr = spc_resolve_addr(mode);
    uint16_t val = spc_read_8(addr);
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    val |= spc_read_8(addr) << 8;
    spc.y = U16_HIBYTE(val);
    spc.a = U16_LOBYTE(val);
    spc_set_status_bit(STATUS_NEGATIVE, val & 0x8000);
    spc_set_status_bit(STATUS_ZERO, val == 0);
}

OP(sta) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_INDIRECT_INC);
    spc_resolve_write(mode, spc.a);
}

OP(stx) {
    LEGALADDRMODES(SM_ABS | SM_DIR_PAGE | SM_DIR_PAGEY);
    spc_resolve_write(mode, spc.x);
}

OP(sty) {
    LEGALADDRMODES(SM_ABS | SM_DIR_PAGE | SM_DIR_PAGEX);
    spc_resolve_write(mode, spc.y);
}

OP(stw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t addr = spc_resolve_addr(mode);
    spc_write_8(addr, spc.a);
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    spc_write_8(addr, spc.y);
}

OP(txs) {
    LEGALADDRMODES(SM_IMP);
    spc.s = spc.x;
}

OP(tsx) {
    LEGALADDRMODES(SM_IMP);
    spc.x = spc.s;
    spc_set_status_bit(STATUS_ZERO, spc.x == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.x & 0x80);
}

OP(txa) {
    LEGALADDRMODES(SM_IMP);
    spc.a = spc.x;
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}

OP(tax) {
    LEGALADDRMODES(SM_IMP);
    spc.x = spc.a;
    spc_set_status_bit(STATUS_ZERO, spc.x == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.x & 0x80);
}

OP(tya) {
    LEGALADDRMODES(SM_IMP);
    spc.a = spc.y;
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}

OP(tay) {
    LEGALADDRMODES(SM_IMP);
    spc.y = spc.a;
    spc_set_status_bit(STATUS_ZERO, spc.y == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.y & 0x80);
}

OP(inx) {
    LEGALADDRMODES(SM_IMP);
    spc.x++;
    spc_set_status_bit(STATUS_ZERO, spc.x == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.x & 0x80);
}

OP(dex) {
    LEGALADDRMODES(SM_IMP);
    spc.x--;
    spc_set_status_bit(STATUS_ZERO, spc.x == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.x & 0x80);
}

OP(iny) {
    LEGALADDRMODES(SM_IMP);
    spc.y++;
    spc_set_status_bit(STATUS_ZERO, spc.y == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.y & 0x80);
}

OP(dey) {
    LEGALADDRMODES(SM_IMP);
    spc.y--;
    spc_set_status_bit(STATUS_ZERO, spc.y == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.y & 0x80);
}

OP(bra) {
    LEGALADDRMODES(SM_REL);
    spc.pc += (int8_t)spc_next_8();
}

OP(beq) {
    LEGALADDRMODES(SM_REL);
    if (spc_get_status_bit(STATUS_ZERO)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bne) {
    LEGALADDRMODES(SM_REL);
    if (!spc_get_status_bit(STATUS_ZERO)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bmi) {
    LEGALADDRMODES(SM_REL);
    if (spc_get_status_bit(STATUS_NEGATIVE)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bpl) {
    LEGALADDRMODES(SM_REL);
    if (!spc_get_status_bit(STATUS_NEGATIVE)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bcc) {
    LEGALADDRMODES(SM_REL);
    if (!spc_get_status_bit(STATUS_CARRY)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bcs) {
    LEGALADDRMODES(SM_REL);
    if (spc_get_status_bit(STATUS_CARRY)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bvs) {
    LEGALADDRMODES(SM_REL);
    if (spc_get_status_bit(STATUS_OVERFLOW)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(bvc) {
    LEGALADDRMODES(SM_REL);
    if (!spc_get_status_bit(STATUS_OVERFLOW)) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

#define OP_BBS(x)                                                              \
    OP(bbs##x) {                                                               \
        LEGALADDRMODES(SM_DIR_PAGE_BIT_REL);                                   \
        uint8_t val = spc_resolve_read(SM_DIR_PAGE);                           \
        if (val & (1 << x)) {                                                  \
            spc.pc += (int8_t)spc_next_8();                                    \
        } else {                                                               \
            spc.pc++;                                                          \
        }                                                                      \
    }
OP_BBS(0);
OP_BBS(1);
OP_BBS(2);
OP_BBS(3);
OP_BBS(4);
OP_BBS(5);
OP_BBS(6);
OP_BBS(7);
#undef OP_BBS

#define OP_BBC(x)                                                              \
    OP(bbc##x) {                                                               \
        LEGALADDRMODES(SM_DIR_PAGE_BIT_REL);                                   \
        uint8_t val = spc_resolve_read(SM_DIR_PAGE);                           \
        if (val & (1 << x)) {                                                  \
            spc.pc++;                                                          \
        } else {                                                               \
            spc.pc += (int8_t)spc_next_8();                                    \
        }                                                                      \
    }
OP_BBC(0);
OP_BBC(1);
OP_BBC(2);
OP_BBC(3);
OP_BBC(4);
OP_BBC(5);
OP_BBC(6);
OP_BBC(7);
#undef OP_BBS

OP(cbne) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX);
    uint8_t operand = spc_resolve_read(mode);
    if (spc.a != operand) {
        spc.pc += (int8_t)spc_next_8();
    } else {
        spc.pc++;
    }
}

OP(dbnz) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_Y);
    if (mode == SM_Y) {
        spc.y--;
        if (spc.y != 0) {
            spc.pc += (int8_t)spc_next_8();
        } else {
            spc.pc++;
        }
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr) - 1;
        spc_write_8(addr, val);
        if (val != 0) {
            spc.pc += (int8_t)spc_next_8();
        } else {
            spc.pc++;
        }
    }
}

OP(jmp) {
    LEGALADDRMODES(SM_ABS | SM_ABS_INDX);
    spc.pc = spc_resolve_addr(mode);
}

OP(jsr) {
    LEGALADDRMODES(SM_ABS);
    spc_push_16(spc.pc + 2);
    spc.pc = spc_resolve_addr(mode);
}

OP(jsp) {
    LEGALADDRMODES(SM_IMP);
    spc_push_16(spc.pc + 1);
    spc.pc = 0xff00 | spc_next_8();
}

#define OP_JST(x)                                                              \
    OP(jst##x) {                                                               \
        LEGALADDRMODES(SM_IMP);                                                \
        spc_push_16(spc.pc);                                                   \
        spc.pc = spc_read_16(0xff00 | (0xde - 0x##x * 2));                     \
    }

OP_JST(0);
OP_JST(1);
OP_JST(2);
OP_JST(3);
OP_JST(4);
OP_JST(5);
OP_JST(6);
OP_JST(7);
OP_JST(8);
OP_JST(9);
OP_JST(a);
OP_JST(b);
OP_JST(c);
OP_JST(d);
OP_JST(e);
OP_JST(f);

#undef OP_JST

OP(rts) {
    LEGALADDRMODES(SM_IMP);
    spc.pc = spc_pop_16();
}

OP(rti) {
    LEGALADDRMODES(SM_IMP);
    spc.p = spc_pop_8();
    spc.pc = spc_pop_16();
}

OP(pha) {
    LEGALADDRMODES(SM_IMP);
    spc_push_8(spc.a);
}

OP(pla) {
    LEGALADDRMODES(SM_IMP);
    spc.a = spc_pop_8();
}

OP(phx) {
    LEGALADDRMODES(SM_IMP);
    spc_push_8(spc.x);
}

OP(plx) {
    LEGALADDRMODES(SM_IMP);
    spc.x = spc_pop_8();
}

OP(phy) {
    LEGALADDRMODES(SM_IMP);
    spc_push_8(spc.y);
}

OP(ply) {
    LEGALADDRMODES(SM_IMP);
    spc.y = spc_pop_8();
}

OP(php) {
    LEGALADDRMODES(SM_IMP);
    spc_push_8(spc.p);
}

OP(plp) {
    LEGALADDRMODES(SM_IMP);
    spc.p = spc_pop_8();
}

OP(mov) {
    LEGALADDRMODES(SM_DIR_PAGE_TO_DIR_PAGE | SM_IMM_TO_DIR_PAGE);
    if (mode == SM_IMM_TO_DIR_PAGE) {
        uint8_t val = spc_next_8();
        uint16_t addr =
            spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        spc_write_8(addr, val);
    } else {
        uint16_t src =
            spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        uint16_t dst =
            spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        spc_write_8(dst, spc_read_8(src));
    }
}

OP(cmp) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_DIR_PAGE_TO_DIR_PAGE | SM_IMM_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op2 = spc_next_8();
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op2 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc.y + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }

    spc_set_status_bit(STATUS_NEGATIVE, (op1 - op2) & 0x80);
    spc_set_status_bit(STATUS_ZERO, op1 == op2);
    spc_set_status_bit(STATUS_CARRY, op1 >= op2);
}

OP(cpx) {
    LEGALADDRMODES(SM_IMM | SM_ABS | SM_DIR_PAGE);
    uint8_t op1 = spc.x;
    uint8_t op2 = spc_resolve_read(mode);
    spc_set_status_bit(STATUS_NEGATIVE, (op1 - op2) & 0x80);
    spc_set_status_bit(STATUS_ZERO, op1 == op2);
    spc_set_status_bit(STATUS_CARRY, op1 >= op2);
}

OP(cpy) {
    LEGALADDRMODES(SM_IMM | SM_ABS | SM_DIR_PAGE);
    uint8_t op1 = spc.y;
    uint8_t op2 = spc_resolve_read(mode);
    spc_set_status_bit(STATUS_NEGATIVE, (op1 - op2) & 0x80);
    spc_set_status_bit(STATUS_ZERO, op1 == op2);
    spc_set_status_bit(STATUS_CARRY, op1 >= op2);
}

OP(inc) {
    LEGALADDRMODES(SM_ABS | SM_DIR_PAGE | SM_ACC | SM_DIR_PAGEX);
    if (mode == SM_ACC) {
        spc.a++;
        spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
        spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr) + 1;
        spc_write_8(addr, val);
        spc_set_status_bit(STATUS_NEGATIVE, val & 0x80);
        spc_set_status_bit(STATUS_ZERO, val == 0);
    }
}

OP(dec) {
    LEGALADDRMODES(SM_ABS | SM_DIR_PAGE | SM_ACC | SM_DIR_PAGEX);
    if (mode == SM_ACC) {
        spc.a--;
        spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
        spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr) - 1;
        spc_write_8(addr, val);
        spc_set_status_bit(STATUS_NEGATIVE, val & 0x80);
        spc_set_status_bit(STATUS_ZERO, val == 0);
    }
}

OP(inw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t addr = spc_resolve_addr(mode);
    uint16_t val = spc_read_8(addr);
    spc_write_8(addr, U16_LOBYTE(val + 1));
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    val |= spc_read_8(addr) << 8;
    val++;
    spc_set_status_bit(STATUS_ZERO, val == 0);
    spc_set_status_bit(STATUS_NEGATIVE, val & 0x8000);
    spc_write_8(addr, U16_HIBYTE(val));
}

OP(dew) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t addr = spc_resolve_addr(mode);
    uint16_t val = spc_read_8(addr);
    spc_write_8(addr, U16_LOBYTE(val - 1));
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    val |= spc_read_8(addr) << 8;
    val--;
    spc_set_status_bit(STATUS_ZERO, val == 0);
    spc_set_status_bit(STATUS_NEGATIVE, val & 0x8000);
    spc_write_8(addr, U16_HIBYTE(val));
}

OP(adw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t op1 = TO_U16(spc.a, spc.y);
    uint16_t addr = spc_resolve_addr(mode);
    uint16_t op2 = spc_read_8(addr);
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    op2 |= spc_read_8(addr) << 8;
    int32_t result = op1 + op2;
    spc_set_status_bit(STATUS_CARRY, result > 0xffff);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x8000);
    spc_set_status_bit(STATUS_OVERFLOW,
                       ~(U16_HIBYTE(op1) ^ U16_HIBYTE(op2)) &
                           (U16_HIBYTE(op1) ^ U16_HIBYTE((uint16_t)result)) &
                           0x80);
    spc_set_status_bit(STATUS_HALFCARRY, (U16_HIBYTE(op1) ^ U16_HIBYTE(op2) ^
                                          U16_HIBYTE((uint16_t)result)) &
                                             0x10);
    spc_set_status_bit(STATUS_ZERO, (result & 0xffff) == 0);
    spc.y = U16_HIBYTE((uint16_t)result);
    spc.a = U16_LOBYTE((uint16_t)result);
}
OP(sbw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t op1 = TO_U16(spc.a, spc.y);
    uint16_t addr = spc_resolve_addr(mode);
    uint16_t op2 = spc_read_8(addr);
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    op2 |= spc_read_8(addr) << 8;
    int32_t result = op1 - op2;
    spc_set_status_bit(STATUS_CARRY, result >= 0);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x8000);
    spc_set_status_bit(STATUS_OVERFLOW,
                       (U16_HIBYTE(op1) ^ U16_HIBYTE(op2)) &
                           (U16_HIBYTE(op1) ^ U16_HIBYTE((uint16_t)result)) &
                           0x80);
    spc_set_status_bit(STATUS_HALFCARRY, !((U16_HIBYTE(op1) ^ U16_HIBYTE(op2) ^
                                            U16_HIBYTE((uint16_t)result)) &
                                           0x10));
    spc_set_status_bit(STATUS_ZERO, (result & 0xffff) == 0);
    spc.y = U16_HIBYTE((uint16_t)result);
    spc.a = U16_LOBYTE((uint16_t)result);
}

OP(cpw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    uint16_t op1 = TO_U16(spc.a, spc.y);
    uint16_t addr = spc_resolve_addr(mode);
    uint16_t op2 = spc_read_8(addr);
    if (addr % 0x100 == 0xff)
        addr -= 0xff;
    else
        addr++;
    op2 |= spc_read_8(addr) << 8;
    int32_t result = op1 - op2;
    spc_set_status_bit(STATUS_CARRY, result >= 0);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x8000);
    spc_set_status_bit(STATUS_ZERO, (result & 0xffff) == 0);
}

OP(adc) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_IMM_TO_DIR_PAGE | SM_DIR_PAGE_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    uint16_t dest = 0;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op1 = spc_next_8();
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc.y + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }
    uint16_t result = op1 + op2 + spc_get_status_bit(STATUS_CARRY);
    spc_set_status_bit(STATUS_CARRY, result > 0xff);
    spc_set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
    spc_set_status_bit(STATUS_HALFCARRY, (result ^ op1 ^ op2) & 0x10);
    spc_set_status_bit(STATUS_OVERFLOW, ~(op1 ^ op2) & (op1 ^ result) & 0x80);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x80);
    if (mode == SM_IMM_TO_DIR_PAGE || mode == SM_DIR_PAGE_TO_DIR_PAGE ||
        mode == SM_IND_PAGE_TO_IND_PAGE) {
        spc_write_8(dest, result);
    } else {
        spc.a = result;
    }
}

OP(sbc) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_IMM_TO_DIR_PAGE | SM_DIR_PAGE_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    uint16_t dest = 0;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op2 = spc_next_8();
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op1 = spc_read_8(dest);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op2 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op1 = spc_read_8(dest);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc.y + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }
    int16_t result = op1 - op2 - !spc_get_status_bit(STATUS_CARRY);
    spc_set_status_bit(STATUS_CARRY, result >= 0);
    spc_set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
    spc_set_status_bit(STATUS_HALFCARRY, !((result ^ op1 ^ op2) & 0x10));
    spc_set_status_bit(STATUS_OVERFLOW, (op1 ^ op2) & (op1 ^ result) & 0x80);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x80);
    if (mode == SM_IMM_TO_DIR_PAGE || mode == SM_DIR_PAGE_TO_DIR_PAGE ||
        mode == SM_IND_PAGE_TO_IND_PAGE) {
        spc_write_8(dest, result);
    } else {
        spc.a = result;
    }
}

OP(and) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_IMM_TO_DIR_PAGE | SM_DIR_PAGE_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    uint16_t dest;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op1 = spc_next_8();
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc.y + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }
    uint16_t result = op1 & op2;
    spc_set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x80);
    if (mode == SM_IMM_TO_DIR_PAGE || mode == SM_DIR_PAGE_TO_DIR_PAGE ||
        mode == SM_IND_PAGE_TO_IND_PAGE) {
        spc_write_8(dest, result);
    } else {
        spc.a = result;
    }
}

OP(ora) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_IMM_TO_DIR_PAGE | SM_DIR_PAGE_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    uint16_t dest;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op1 = spc_next_8();
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc.y + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }
    uint16_t result = op1 | op2;
    spc_set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x80);
    if (mode == SM_IMM_TO_DIR_PAGE || mode == SM_DIR_PAGE_TO_DIR_PAGE ||
        mode == SM_IND_PAGE_TO_IND_PAGE) {
        spc_write_8(dest, result);
    } else {
        spc.a = result;
    }
}

OP(eor) {
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | SM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_IMM_TO_DIR_PAGE | SM_DIR_PAGE_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    uint16_t dest;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op1 = spc_next_8();
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
        op2 = spc_read_8(dest);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc.y + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        dest = spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }
    uint16_t result = op1 ^ op2;
    spc_set_status_bit(STATUS_ZERO, (result & 0xff) == 0);
    spc_set_status_bit(STATUS_NEGATIVE, result & 0x80);
    if (mode == SM_IMM_TO_DIR_PAGE || mode == SM_DIR_PAGE_TO_DIR_PAGE ||
        mode == SM_IND_PAGE_TO_IND_PAGE) {
        spc_write_8(dest, result);
    } else {
        spc.a = result;
    }
}

OP(mul) {
    LEGALADDRMODES(SM_IMP);
    uint16_t result = spc.a * spc.y;
    spc.a = U16_LOBYTE(result);
    spc.y = U16_HIBYTE(result);
    spc_set_status_bit(STATUS_ZERO, spc.y == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.y & 0x80);
}

OP(div) {
    LEGALADDRMODES(SM_IMP);
    uint8_t result1, result2;
    if (spc.y < (spc.x << 1)) {
        result1 = TO_U16(spc.a, spc.y) / spc.x;
        result2 = TO_U16(spc.a, spc.y) % spc.x;
    } else {
        result1 = 255 - (TO_U16(spc.a, spc.y) - (spc.x << 9)) / (256 - spc.x);
        result2 = spc.x + (TO_U16(spc.a, spc.y) - (spc.x << 9)) % (256 - spc.x);
    }
    spc_set_status_bit(STATUS_OVERFLOW, spc.y >= spc.x);
    spc_set_status_bit(STATUS_HALFCARRY, (spc.y & 0xf) >= (spc.x & 0xf));
    spc.a = result1;
    spc.y = result2;
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}

OP(asl) {
    LEGALADDRMODES(SM_ACC | SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS);
    if (mode == SM_ACC) {
        spc_set_status_bit(STATUS_CARRY, spc.a & 0x80);
        spc.a <<= 1;
        spc_set_status_bit(STATUS_ZERO, spc.a == 0);
        spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr);
        spc_set_status_bit(STATUS_CARRY, val & 0x80);
        val <<= 1;
        spc_set_status_bit(STATUS_ZERO, val == 0);
        spc_set_status_bit(STATUS_NEGATIVE, val & 0x80);
        spc_write_8(addr, val);
    }
}

OP(lsr) {
    LEGALADDRMODES(SM_ACC | SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS);
    if (mode == SM_ACC) {
        spc_set_status_bit(STATUS_CARRY, spc.a & 1);
        spc.a >>= 1;
        spc_set_status_bit(STATUS_ZERO, spc.a == 0);
        spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr);
        spc_set_status_bit(STATUS_CARRY, val & 1);
        val >>= 1;
        spc_set_status_bit(STATUS_ZERO, val == 0);
        spc_set_status_bit(STATUS_NEGATIVE, val & 0x80);
        spc_write_8(addr, val);
    }
}

OP(ror) {
    LEGALADDRMODES(SM_ACC | SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS);
    bool c = spc_get_status_bit(STATUS_CARRY);
    if (mode == SM_ACC) {
        spc_set_status_bit(STATUS_CARRY, spc.a & 1);
        spc.a >>= 1;
        spc.a |= c << 7;
        spc_set_status_bit(STATUS_ZERO, spc.a == 0);
        spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr);
        spc_set_status_bit(STATUS_CARRY, val & 1);
        val >>= 1;
        val |= c << 7;
        spc_set_status_bit(STATUS_ZERO, val == 0);
        spc_set_status_bit(STATUS_NEGATIVE, val & 0x80);
        spc_write_8(addr, val);
    }
}

OP(rol) {
    LEGALADDRMODES(SM_ACC | SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS);
    bool c = spc_get_status_bit(STATUS_CARRY);
    if (mode == SM_ACC) {
        spc_set_status_bit(STATUS_CARRY, spc.a & 0x80);
        spc.a <<= 1;
        spc.a |= c;
        spc_set_status_bit(STATUS_ZERO, spc.a == 0);
        spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
    } else {
        uint16_t addr = spc_resolve_addr(mode);
        uint8_t val = spc_read_8(addr);
        spc_set_status_bit(STATUS_CARRY, val & 0x80);
        val <<= 1;
        val |= c;
        spc_set_status_bit(STATUS_ZERO, val == 0);
        spc_set_status_bit(STATUS_NEGATIVE, val & 0x80);
        spc_write_8(addr, val);
    }
}

OP(sep) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_DIRECTPAGE, true);
}

OP(clp) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_DIRECTPAGE, false);
}

OP(sec) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_CARRY, true);
}

OP(clc) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_CARRY, false);
}

OP(sei) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_IRQOFF, false);
}

OP(cli) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_IRQOFF, true);
}

OP(clv) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_OVERFLOW, false);
    spc_set_status_bit(STATUS_HALFCARRY, false);
}

OP(notc) {
    LEGALADDRMODES(SM_IMP);
    spc_set_status_bit(STATUS_CARRY, !spc_get_status_bit(STATUS_CARRY));
}

#define OP_SET(x)                                                              \
    OP(set##x) {                                                               \
        LEGALADDRMODES(SM_DIR_PAGE_BIT);                                       \
        uint16_t addr = spc_resolve_addr(SM_DIR_PAGE);                         \
        spc_write_8(addr, spc_read_8(addr) | (1 << x));                        \
    }

OP_SET(0);
OP_SET(1);
OP_SET(2);
OP_SET(3);
OP_SET(4);
OP_SET(5);
OP_SET(6);
OP_SET(7);

#undef OP_SET

#define OP_CLR(x)                                                              \
    OP(clr##x) {                                                               \
        LEGALADDRMODES(SM_DIR_PAGE_BIT);                                       \
        uint16_t addr = spc_resolve_addr(SM_DIR_PAGE);                         \
        spc_write_8(addr, spc_read_8(addr) & ~(1 << x));                       \
    }

OP_CLR(0);
OP_CLR(1);
OP_CLR(2);
OP_CLR(3);
OP_CLR(4);
OP_CLR(5);
OP_CLR(6);
OP_CLR(7);

#undef OP_CLR

OP(and1) {
    LEGALADDRMODES(SM_ABS_BOOL_BIT | SM_ABS_BOOL_BIT_INV);
    uint16_t addr = spc_next_16();
    uint8_t bit = addr >> 13;
    addr &= 0x1fff;
    if (mode == SM_ABS_BOOL_BIT) {
        spc_set_status_bit(STATUS_CARRY,
                           spc_get_status_bit(STATUS_CARRY) &&
                               (((spc_read_8(addr) >> bit) & 1) == 1));
    } else {
        spc_set_status_bit(STATUS_CARRY,
                           spc_get_status_bit(STATUS_CARRY) &&
                               (((spc_read_8(addr) >> bit) & 1) == 0));
    }
}

OP(or1) {
    LEGALADDRMODES(SM_ABS_BOOL_BIT | SM_ABS_BOOL_BIT_INV);
    uint16_t addr = spc_next_16();
    uint8_t bit = addr >> 13;
    addr &= 0x1fff;
    if (mode == SM_ABS_BOOL_BIT) {
        spc_set_status_bit(STATUS_CARRY,
                           spc_get_status_bit(STATUS_CARRY) ||
                               (((spc_read_8(addr) >> bit) & 1) == 1));
    } else {
        spc_set_status_bit(STATUS_CARRY,
                           spc_get_status_bit(STATUS_CARRY) ||
                               (((spc_read_8(addr) >> bit) & 1) == 0));
    }
}

OP(eor1) {
    LEGALADDRMODES(SM_ABS_BOOL_BIT);
    uint16_t addr = spc_next_16();
    uint8_t bit = addr >> 13;
    addr &= 0x1fff;
    if (mode == SM_ABS_BOOL_BIT) {
        spc_set_status_bit(STATUS_CARRY,
                           spc_get_status_bit(STATUS_CARRY) ^
                               (((spc_read_8(addr) >> bit) & 1) == 1));
    } else {
        spc_set_status_bit(STATUS_CARRY,
                           spc_get_status_bit(STATUS_CARRY) ^
                               (((spc_read_8(addr) >> bit) & 1) == 0));
    }
}

OP(ld1) {
    LEGALADDRMODES(SM_ABS_BOOL_BIT);
    uint16_t addr = spc_next_16();
    uint8_t bit = addr >> 13;
    addr &= 0x1fff;
    spc_set_status_bit(STATUS_CARRY, (spc_read_8(addr) >> bit) & 1);
}

OP(st1) {
    LEGALADDRMODES(SM_ABS_BOOL_BIT);
    uint16_t addr = spc_next_16();
    uint8_t bit = addr >> 13;
    addr &= 0x1fff;
    uint8_t val = spc_read_8(addr);
    val &= ~(1 << bit);
    val |= (spc_get_status_bit(STATUS_CARRY) << bit);
    spc_write_8(addr, val);
}

OP(not1) {
    LEGALADDRMODES(SM_ABS_BOOL_BIT);
    uint16_t addr = spc_next_16();
    uint8_t bit = addr >> 13;
    addr &= 0x1fff;
    uint8_t val = spc_read_8(addr);
    val ^= (1 << bit);
    spc_write_8(addr, val);
}

OP(tset1) {
    LEGALADDRMODES(SM_ABS);
    uint16_t addr = spc_resolve_addr(mode);
    uint8_t val = spc_read_8(addr);
    spc_set_status_bit(STATUS_ZERO, val == spc.a);
    spc_set_status_bit(STATUS_NEGATIVE, (spc.a - val) & 0x80);
    val |= spc.a;
    spc_write_8(addr, val);
}

OP(tclr1) {
    LEGALADDRMODES(SM_ABS);
    uint16_t addr = spc_resolve_addr(mode);
    uint8_t val = spc_read_8(addr);
    spc_set_status_bit(STATUS_ZERO, val == spc.a);
    spc_set_status_bit(STATUS_NEGATIVE, (spc.a - val) & 0x80);
    val &= ~spc.a;
    spc_write_8(addr, val);
}

OP(xcn) {
    LEGALADDRMODES(SM_ACC);
    spc.a = ((spc.a & 0xf) << 4) | (spc.a >> 4);
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}

OP(brk) {
    LEGALADDRMODES(SM_IMP);
    spc.brk = true;
}

OP(daa) {
    LEGALADDRMODES(SM_IMP);
    if (spc_get_status_bit(STATUS_CARRY) || spc.a > 0x99) {
        spc.a += 0x60;
        spc_set_status_bit(STATUS_CARRY, true);
    }

    if (spc_get_status_bit(STATUS_HALFCARRY) || (spc.a & 0xf) > 0x9) {
        spc.a += 6;
    }
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}

OP(das) {
    LEGALADDRMODES(SM_IMP);
    if (!spc_get_status_bit(STATUS_CARRY) || spc.a > 0x99) {
        spc.a -= 0x60;
        spc_set_status_bit(STATUS_CARRY, false);
    }

    if (!spc_get_status_bit(STATUS_HALFCARRY) || (spc.a & 0xf) > 0x9) {
        spc.a -= 6;
    }
    spc_set_status_bit(STATUS_ZERO, spc.a == 0);
    spc_set_status_bit(STATUS_NEGATIVE, spc.a & 0x80);
}
