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
                   SM_DIR_PAGEX | SM_INDIRECT | SM_INDIRECT | SM_INDX |
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
    uint16_t val = spc_read_16(spc_resolve_addr(mode));
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

OP(sty) {
    LEGALADDRMODES(SM_ABS | SM_DIR_PAGE | SM_DIR_PAGEX);
    spc_resolve_write(mode, spc.y);
}

OP(stw) {
    LEGALADDRMODES(SM_DIR_PAGE);
    spc_write_16(spc_resolve_addr(mode), TO_U16(spc.a, spc.y));
}

OP(txs) {
    LEGALADDRMODES(SM_IMP);
    spc.s = spc.x;
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

OP(bne) {
    LEGALADDRMODES(SM_REL);
    if (!spc_get_status_bit(STATUS_ZERO)) {
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

OP(jmp) {
    LEGALADDRMODES(SM_ABS | SM_ABS_INDX);
    spc.pc = spc_resolve_addr(mode);
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
    LEGALADDRMODES(SM_DIR_PAGE | SM_DIR_PAGEX | SM_ABS | SM_ABSX | AM_ABSY |
                   SM_INDIRECT | SM_INDX | SM_INDY | SM_IMM |
                   SM_DIR_PAGE_TO_DIR_PAGE | SM_IMM_TO_DIR_PAGE |
                   SM_IND_PAGE_TO_IND_PAGE);
    uint8_t op1, op2;
    if (mode == SM_IMM_TO_DIR_PAGE) {
        op1 = spc_next_8();
        op2 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    } else if (mode == SM_DIR_PAGE_TO_DIR_PAGE) {
        op1 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
        op2 = spc_read_8(spc_next_8() +
                         spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    } else if (mode == SM_IND_PAGE_TO_IND_PAGE) {
        op1 = spc_read_8(spc.x);
        op2 = spc_read_8(spc.y);
        TODO("check if indirect access is susceptible to direct page bit");
    } else {
        op1 = spc.a;
        op2 = spc_resolve_read(mode);
    }

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
