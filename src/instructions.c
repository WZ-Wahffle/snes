#include "instructions.h"
#include "cpu.h"
#include "types.h"

extern cpu_t cpu;

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

OP(stz) {
    LEGALADDRMODES(AM_ABS | AM_ABSX | AM_DIR | AM_ZBKX_DIR);

    uint32_t addr = resolve_addr(mode);
    write_8(U24_LOSHORT(addr), U24_HIBYTE(addr), 0);
}
