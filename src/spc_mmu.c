#include "spc_mmu.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

uint8_t ipl_boot_rom[] = {
    0xcd, 0xef, 0xbd, 0xe8, 0x00, 0xc6, 0x1d, 0xd0, 0xfc, 0x8f, 0xaa,
    0xf4, 0x8f, 0xbb, 0xf5, 0x78, 0xcc, 0xf4, 0xd0, 0xfb, 0x2f, 0x19,
    0xeb, 0xf4, 0xd0, 0xfc, 0x7e, 0xf4, 0xd0, 0x0b, 0xe4, 0xf5, 0xcb,
    0xf4, 0xd7, 0x00, 0xfc, 0xd0, 0xf3, 0xab, 0x01, 0x10, 0xef, 0x7e,
    0xf4, 0x10, 0xeb, 0xba, 0xf6, 0xda, 0x00, 0xba, 0xf4, 0xc4, 0xf4,
    0xdd, 0x5d, 0xd0, 0xdb, 0x1f, 0x00, 0x00, 0xc0, 0xff};

uint8_t spc_mmu_read(uint16_t addr, bool log) {
    (void)log;
    if (addr >= 0xffc0 && spc.enable_ipl) {
        return ipl_boot_rom[addr - 0xffc0];
    } else if (addr >= 0xf0 && addr < 0x100) {
        switch (addr) {
        case 0xf2:
        case 0xf3:
            if (log)
                log_message(LOG_LEVEL_INFO, "Read from DSP %s\n",
                            addr == 0xf2 ? "addr" : "data");
            break;
        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
            return cpu.memory.apu_io[addr - 0xf4];
        case 0xfd:
        case 0xfe:
        case 0xff: {
            uint8_t ret = spc.memory.timers[addr - 0xfd].counter;
            spc.memory.timers[addr - 0xfd].counter = 0;
            return ret;
        }
        default:
            UNREACHABLE_SWITCH(addr);
        }
    }

    return spc.memory.ram[addr];
}

void spc_mmu_write(uint16_t addr, uint8_t val, bool log) {
    (void)log;
    spc.memory.ram[addr] = val;
    if (addr >= 0xf0 && addr < 0x100) {
        switch (addr) {
        case 0xf1:
            spc.enable_ipl = val & 0x80;
            spc.memory.timers[2].enable = val & 0x4;
            spc.memory.timers[1].enable = val & 0x2;
            spc.memory.timers[0].enable = val & 0x1;
            if (val & 0x20) {
                spc.memory.ram[0xf7] = 0;
                spc.memory.ram[0xf6] = 0;
            }
            if (val & 0x10) {
                spc.memory.ram[0xf5] = 0;
                spc.memory.ram[0xf4] = 0;
            }
            break;
        case 0xf2:
        case 0xf3:
            if (log)
                log_message(LOG_LEVEL_INFO, "Write of 0x%02x to DSP %s", val,
                            addr == 0xf2 ? "addr" : "data");
            break;
        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
            if (log)
                log_message(LOG_LEVEL_VERBOSE, "Write of 0x%02x to CPU port %d",
                            val, addr - 0xf4 + 1);
            break;
        case 0xfa:
        case 0xfb:
        case 0xfc:
            spc.memory.timers[addr - 0xfa].timer = val;
            break;
        default:
            UNREACHABLE_SWITCH(addr);
        }
    }
}
