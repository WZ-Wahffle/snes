#include "spc.h"
#include "spc_instructions.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

uint8_t spc_read_8(uint16_t addr) { return spc_mmu_read(addr, true); }

uint8_t spc_read_8_no_log(uint16_t addr) { return spc_mmu_read(addr, false); }

uint16_t spc_read_16(uint16_t addr) {
    uint8_t lsb = spc_read_8(addr);
    return TO_U16(lsb, spc_read_8(addr + 1));
}

uint8_t spc_next_8(void) { return spc_read_8(spc.pc++); }

uint16_t spc_next_16(void) {
    uint8_t lsb = spc_next_8();
    return TO_U16(lsb, spc_next_8());
}

void spc_write_8(uint16_t addr, uint8_t val) { spc_mmu_write(addr, val, true); }

void spc_write_16(uint16_t addr, uint16_t val) {
    spc_write_8(addr, U16_LOBYTE(val));
    spc_write_8(addr + 1, U16_HIBYTE(val));
}

void spc_set_status_bit(status_bit_t bit, bool value) {
    log_message(LOG_LEVEL_VERBOSE, "SPC set status bit %d", bit);
    if (value) {
        spc.p |= 1 << bit;
    } else {
        spc.p &= ~(1 << bit);
    }
}

bool spc_get_status_bit(status_bit_t bit) { return spc.p & (1 << bit); }

void spc_push_8(uint8_t val) { spc_write_8(0x100 + (spc.s--), val); }

void spc_push_16(uint16_t val) {
    spc_push_8(U16_HIBYTE(val));
    spc_push_8(U16_LOBYTE(val));
}

uint8_t spc_pop_8(void) { return spc_read_8(0x100 + (++spc.s)); }

uint16_t spc_pop_16(void) {
    uint8_t lsb = spc_pop_8();
    uint8_t msb = spc_pop_8();
    return TO_U16(lsb, msb);
}

uint8_t spc_resolve_read(spc_addressing_mode_t mode) {
    switch (mode) {
    case SM_IMM:
        return spc_next_8();
    case SM_ABS:
        return spc_read_8(spc_next_16());
    case SM_ABSX:
        return spc_read_8(spc_next_16() + spc.x);
    case SM_ABSY:
        return spc_read_8(spc_next_16() + spc.y);
    case SM_DIR_PAGE:
        return spc_read_8(spc_next_8() +
                          spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    case SM_DIR_PAGEX:
        return spc_read_8((spc_next_8() + spc.x) % 0x100 +
                          spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    case SM_DIR_PAGEY:
        return spc_read_8((spc_next_8() + spc.y) % 0x100 +
                          spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    case SM_INDX:
        return spc_read_8(spc_read_16(spc_next_8() + spc.x));
    case SM_INDY:
        return spc_read_8(spc_read_16(spc_next_8()) + spc.y);
    default:
        UNREACHABLE_SWITCH(mode);
    }
}

uint16_t spc_resolve_addr(spc_addressing_mode_t mode) {
    switch (mode) {
    case SM_ABS:
        return spc_next_16();
    case SM_DIR_PAGE:
        return spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    case SM_DIR_PAGEX:
        return (spc_next_8() + spc.x) % 0x100 +
               spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    case SM_DIR_PAGEY:
        return (spc_next_8() + spc.y) % 0x100 +
               spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100;
    case SM_ABS_INDX:
        return spc_read_16(spc_next_16() + spc.x);
    default:
        UNREACHABLE_SWITCH(mode);
    }
}

void spc_resolve_write(spc_addressing_mode_t mode, uint8_t val) {
    switch (mode) {
    case SM_DIR_PAGE:
        spc_write_8(
            spc_next_8() + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100, val);
        break;
    case SM_DIR_PAGEX:
        spc_write_8((spc_next_8() + spc.x) % 0x100 +
                        spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100,
                    val);
        break;
    case SM_ABS:
        spc_write_8(spc_next_16(), val);
        break;
    case SM_ABSX:
        spc_write_8(spc_next_16() + spc.x, val);
        break;
    case SM_ABSY:
        spc_write_8(spc_next_16() + spc.y, val);
        break;
    case SM_INDIRECT:
        spc_write_8(spc.x + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100, val);
        break;
    case SM_INDIRECT_INC:
        spc_write_8(spc.x++ + spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100,
                    val);
        break;
    case SM_INDY:
        spc_write_8(spc.y + spc_read_16(spc_next_8()), val);
        break;
    default:
        UNREACHABLE_SWITCH(mode);
    }
}

void spc_reset(void) {
    spc.enable_ipl = true;
    spc.pc = spc_read_16(0xfffe);
}

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
    static uint8_t timer_timer = 0, fast_timer_timer = 0;
    uint8_t opcode = spc_next_8();
    log_message(LOG_LEVEL_VERBOSE, "SPC fetched opcode 0x%02x", opcode);
    // The SPC700 technically resides on its own clock, but this would make
    // synchronization awkward in emulation, so instead cycle counts are
    // multiplied by 21 which is roughly accurate to the lower clock frequency
    spc.remaining_clocks -= 21 * spc_cycle_counts[opcode];
    timer_timer += spc_cycle_counts[opcode];
    fast_timer_timer += spc_cycle_counts[opcode];
    if (fast_timer_timer >= 16) {
        if (spc.memory.timers[2].enable) {
            spc.memory.timers[2].timer_internal++;
            if (spc.memory.timers[2].timer_internal ==
                spc.memory.timers[2].timer) {
                spc.memory.timers[2].counter++;
                spc.memory.timers[2].counter %= 16;
                spc.memory.timers[2].timer_internal = 0;
            }
        }
        fast_timer_timer -= 16;
    }
    if (timer_timer >= 128) {
        if (spc.memory.timers[0].enable) {
            spc.memory.timers[0].timer_internal++;
            if (spc.memory.timers[0].timer_internal ==
                spc.memory.timers[0].timer) {
                spc.memory.timers[0].counter++;
                spc.memory.timers[0].counter %= 16;
                spc.memory.timers[0].timer_internal = 0;
            }
        }
        if (spc.memory.timers[1].enable) {
            spc.memory.timers[1].timer_internal++;
            if (spc.memory.timers[1].timer_internal ==
                spc.memory.timers[1].timer) {
                spc.memory.timers[1].counter++;
                spc.memory.timers[1].counter %= 16;
                spc.memory.timers[1].timer_internal = 0;
            }
        }
        timer_timer -= 128;
    }
    switch (opcode) {
    case 0x03:
        spc_bbs0(SM_DIR_PAGE_BIT_REL);
        break;
    case 0x08:
        spc_ora(SM_IMM);
        break;
    case 0x09:
        spc_ora(SM_DIR_PAGE_TO_DIR_PAGE);
        break;
    case 0x0a:
        spc_or1(SM_ABS_BOOL_BIT);
        break;
    case 0x0b:
        spc_asl(SM_DIR_PAGE);
        break;
    case 0x10:
        spc_bpl(SM_REL);
        break;
    case 0x1a:
        spc_dew(SM_DIR_PAGE);
        break;
    case 0x1c:
        spc_asl(SM_ACC);
        break;
    case 0x1d:
        spc_dex(SM_IMP);
        break;
    case 0x1f:
        spc_jmp(SM_ABS_INDX);
        break;
    case 0x20:
        spc_clp(SM_IMP);
        break;
    case 0x24:
        spc_and(SM_DIR_PAGE);
        break;
    case 0x28:
        spc_and(SM_IMM);
        break;
    case 0x2a:
        spc_or1(SM_ABS_BOOL_BIT_INV);
        break;
    case 0x2d:
        spc_pha(SM_IMP);
        break;
    case 0x2e:
        spc_cbne(SM_DIR_PAGE);
        break;
    case 0x2f:
        spc_bra(SM_REL);
        break;
    case 0x30:
        spc_bmi(SM_REL);
        break;
    case 0x3a:
        spc_inw(SM_DIR_PAGE);
        break;
    case 0x3d:
        spc_inx(SM_IMP);
        break;
    case 0x3f:
        spc_jsr(SM_ABS);
        break;
    case 0x40:
        spc_sep(SM_IMP);
        break;
    case 0x48:
        spc_eor(SM_IMM);
        break;
    case 0x4b:
        spc_lsr(SM_DIR_PAGE);
        break;
    case 0x4d:
        spc_phx(SM_IMP);
        break;
    case 0x5c:
        spc_lsr(SM_ACC);
        break;
    case 0x5d:
        spc_tax(SM_IMP);
        break;
    case 0x5e:
        spc_cpy(SM_ABS);
        break;
    case 0x5f:
        spc_jmp(SM_ABS);
        break;
    case 0x60:
        spc_clc(SM_IMP);
        break;
    case 0x65:
        spc_cmp(SM_ABS);
        break;
    case 0x68:
        spc_cmp(SM_IMM);
        break;
    case 0x6d:
        spc_phy(SM_IMP);
        break;
    case 0x6e:
        spc_dbnz(SM_DIR_PAGE);
        break;
    case 0x6f:
        spc_rts(SM_IMP);
        break;
    case 0x74:
        spc_cmp(SM_DIR_PAGEX);
        break;
    case 0x75:
        spc_cmp(SM_ABSX);
        break;
    case 0x78:
        spc_cmp(SM_IMM_TO_DIR_PAGE);
        break;
    case 0x7a:
        spc_adw(SM_DIR_PAGE);
        break;
    case 0x7c:
        spc_ror(SM_ACC);
        break;
    case 0x7d:
        spc_txa(SM_IMP);
        break;
    case 0x7e:
        spc_cpy(SM_DIR_PAGE);
        break;
    case 0x80:
        spc_sec(SM_IMP);
        break;
    case 0x82:
        spc_set4(SM_DIR_PAGE_BIT);
        break;
    case 0x84:
        spc_adc(SM_DIR_PAGE);
        break;
    case 0x85:
        spc_adc(SM_ABS);
        break;
    case 0x88:
        spc_adc(SM_IMM);
        break;
    case 0x8c:
        spc_dec(SM_ABS);
        break;
    case 0x8d:
        spc_ldy(SM_IMM);
        break;
    case 0x8f:
        spc_mov(SM_IMM_TO_DIR_PAGE);
        break;
    case 0x90:
        spc_bcc(SM_REL);
        break;
    case 0x92:
        spc_clr4(SM_DIR_PAGE_BIT);
        break;
    case 0x95:
        spc_adc(SM_ABSX);
        break;
    case 0x96:
        spc_adc(SM_ABSY);
        break;
    case 0x97:
        spc_adc(SM_INDY);
        break;
    case 0x9a:
        spc_sbw(SM_DIR_PAGE);
        break;
    case 0x9b:
        spc_dec(SM_DIR_PAGEX);
        break;
    case 0x9c:
        spc_dec(SM_ACC);
        break;
    case 0x9e:
        spc_div(SM_IMP);
        break;
    case 0x9f:
        spc_xcn(SM_ACC);
        break;
    case 0xa2:
        spc_set5(SM_DIR_PAGE_BIT);
        break;
    case 0xa4:
        spc_sbc(SM_DIR_PAGE);
        break;
    case 0xa8:
        spc_sbc(SM_IMM);
        break;
    case 0xab:
        spc_inc(SM_DIR_PAGE);
        break;
    case 0xad:
        spc_cpy(SM_IMM);
        break;
    case 0xae:
        spc_pla(SM_IMP);
        break;
    case 0xaf:
        spc_sta(SM_INDIRECT_INC);
        break;
    case 0xb0:
        spc_bcs(SM_REL);
        break;
    case 0xb2:
        spc_clr5(SM_DIR_PAGE_BIT);
        break;
    case 0xb5:
        spc_sbc(SM_ABSX);
        break;
    case 0xba:
        spc_ldw(SM_DIR_PAGE);
        break;
    case 0xbb:
        spc_inc(SM_DIR_PAGEX);
        break;
    case 0xbc:
        spc_inc(SM_ACC);
        break;
    case 0xbd:
        spc_txs(SM_IMP);
        break;
    case 0xc2:
        spc_set6(SM_DIR_PAGE_BIT);
        break;
    case 0xc4:
        spc_sta(SM_DIR_PAGE);
        break;
    case 0xc5:
        spc_sta(SM_ABS);
        break;
    case 0xc6:
        spc_sta(SM_INDIRECT);
        break;
    case 0xc8:
        spc_cpx(SM_IMM);
        break;
    case 0xc9:
        spc_stx(SM_ABS);
        break;
    case 0xcb:
        spc_sty(SM_DIR_PAGE);
        break;
    case 0xcc:
        spc_sty(SM_ABS);
        break;
    case 0xcd:
        spc_ldx(SM_IMM);
        break;
    case 0xce:
        spc_plx(SM_IMP);
        break;
    case 0xcf:
        spc_mul(SM_IMP);
        break;
    case 0xd0:
        spc_bne(SM_REL);
        break;
    case 0xd2:
        spc_clr6(SM_DIR_PAGE_BIT);
        break;
    case 0xd4:
        spc_sta(SM_DIR_PAGEX);
        break;
    case 0xd5:
        spc_sta(SM_ABSX);
        break;
    case 0xd6:
        spc_sta(SM_ABSY);
        break;
    case 0xd7:
        spc_sta(SM_INDY);
        break;
    case 0xd8:
        spc_stx(SM_DIR_PAGE);
        break;
    case 0xda:
        spc_stw(SM_DIR_PAGE);
        break;
    case 0xdb:
        spc_sty(SM_DIR_PAGEX);
        break;
    case 0xdc:
        spc_dey(SM_IMP);
        break;
    case 0xdd:
        spc_tya(SM_IMP);
        break;
    case 0xde:
        spc_cbne(SM_DIR_PAGEX);
        break;
    case 0xe2:
        spc_set7(SM_DIR_PAGE_BIT);
        break;
    case 0xe3:
        spc_bbs7(SM_DIR_PAGE_BIT_REL);
        break;
    case 0xe4:
        spc_lda(SM_DIR_PAGE);
        break;
    case 0xe5:
        spc_lda(SM_ABS);
        break;
    case 0xe7:
        spc_lda(SM_INDX);
        break;
    case 0xe8:
        spc_lda(SM_IMM);
        break;
    case 0xeb:
        spc_ldy(SM_DIR_PAGE);
        break;
    case 0xec:
        spc_ldy(SM_ABS);
        break;
    case 0xee:
        spc_ply(SM_IMP);
        break;
    case 0xf0:
        spc_beq(SM_REL);
        break;
    case 0xf2:
        spc_clr7(SM_DIR_PAGE_BIT);
        break;
    case 0xf3:
        spc_bbc7(SM_DIR_PAGE_BIT_REL);
        break;
    case 0xf4:
        spc_lda(SM_DIR_PAGEX);
        break;
    case 0xf5:
        spc_lda(SM_ABSX);
        break;
    case 0xf6:
        spc_lda(SM_ABSY);
        break;
    case 0xf7:
        spc_lda(SM_INDY);
        break;
    case 0xf8:
        spc_ldx(SM_DIR_PAGE);
        break;
    case 0xfb:
        spc_ldy(SM_DIR_PAGEX);
        break;
    case 0xfc:
        spc_iny(SM_IMP);
        break;
    case 0xfd:
        spc_tay(SM_IMP);
        break;
    case 0xfe:
        spc_dbnz(SM_Y);
        break;
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
