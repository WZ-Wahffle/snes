#include "spc.h"
#include "spc_instructions.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

uint8_t spc_read_8(uint16_t addr) { return spc_mmu_read(addr); }

uint16_t spc_read_16(uint16_t addr) {
    uint8_t lsb = spc_read_8(addr);
    return TO_U16(lsb, spc_read_8(addr + 1));
}

uint8_t spc_next_8(void) { return spc_read_8(spc.pc++); }

uint16_t spc_next_16(void) {
    uint8_t lsb = spc_next_8();
    return TO_U16(lsb, spc_next_8());
}

void spc_write_8(uint16_t addr, uint8_t val) { spc_mmu_write(addr, val); }

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

uint8_t spc_resolve_read(spc_addressing_mode_t mode) {
    switch (mode) {
    case SM_IMM:
        return spc_next_8();
    case SM_ABS:
        return spc_read_8(spc_next_16());
    case SM_DIR_PAGE:
        return spc_read_8(spc_next_8() +
                          spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    case SM_DIR_PAGEX:
        return spc_read_8((spc_next_8() + spc.x) % 0x100 +
                          spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
    case SM_DIR_PAGEY:
        return spc_read_8((spc_next_8() + spc.y) % 0x100 +
                          spc_get_status_bit(STATUS_DIRECTPAGE) * 0x100);
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
    case SM_INDY:
        spc_write_8(spc.y + spc_read_16(spc_next_8()), val);
        break;
    default:
        UNREACHABLE_SWITCH(mode);
    }
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
    case 0x10:
        spc_bpl(SM_REL);
        break;
    case 0x1d:
        spc_dex(SM_IMP);
        break;
    case 0x1f:
        spc_jmp(SM_ABS_INDX);
        break;
    case 0x2f:
        spc_bra(SM_REL);
        break;
    case 0x5d:
        spc_tax(SM_IMP);
        break;
    case 0x78:
        spc_cmp(SM_IMM_TO_DIR_PAGE);
        break;
    case 0x7e:
        spc_cpy(SM_DIR_PAGE);
        break;
    case 0x8f:
        spc_mov(SM_IMM_TO_DIR_PAGE);
        break;
    case 0xab:
        spc_inc(SM_DIR_PAGE);
        break;
    case 0xba:
        spc_ldw(SM_DIR_PAGE);
        break;
    case 0xbd:
        spc_txs(SM_IMP);
        break;
    case 0xc4:
        spc_sta(SM_DIR_PAGE);
        break;
    case 0xc6:
        spc_sta(SM_INDIRECT);
        break;
    case 0xcb:
        spc_sty(SM_DIR_PAGE);
        break;
    case 0xcd:
        spc_ldx(SM_IMM);
        break;
    case 0xd0:
        spc_bne(SM_REL);
        break;
    case 0xd7:
        spc_sta(SM_INDY);
        break;
    case 0xda:
        spc_stw(SM_DIR_PAGE);
        break;
    case 0xdd:
        spc_tya(SM_IMP);
        break;
    case 0xe4:
        spc_lda(SM_DIR_PAGE);
        break;
    case 0xe8:
        spc_lda(SM_IMM);
        break;
    case 0xeb:
        spc_ldy(SM_DIR_PAGE);
        break;
    case 0xfc:
        spc_iny(SM_IMP);
        break;
    default:
        UNREACHABLE_SWITCH(opcode);
    }
}
