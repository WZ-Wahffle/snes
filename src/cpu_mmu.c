#include "cpu_mmu.h"
#include "types.h"

extern cpu_t cpu;

// https://snes.nesdev.org/wiki/Memory_map#LoROM
static uint32_t lo_rom_resolve(uint32_t addr) {
    uint32_t ret = addr & 0b11111111111111;
    ret |= (addr >> 1) & 0b111111100000000000000;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    return ret;
}

// https://snes.nesdev.org/wiki/Memory_map#HiROM
static uint32_t hi_rom_resolve(uint32_t addr) {
    uint32_t ret = addr & 0x3fffff;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    return ret;
}

// https://snes.nesdev.org/wiki/Memory_map#ExHiROM
static uint32_t ex_hi_rom_resolve(uint32_t addr) {
    uint32_t ret = addr & 0x3fffff;
    ret |= ((~addr) >> 1) & 0x400000;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    return ret;
}

uint8_t mmu_read(uint16_t addr, uint8_t bank) {
    // ROM resolution
    switch (cpu.memory.mode) {
    case LOROM:
        if ((bank <= 0x7d || bank >= 0x80) && addr >= 0x8000) {
            return cpu.memory.rom[lo_rom_resolve(addr)];
        }
    case HIROM:
        if ((bank < 0x40 && addr >= 0x8000) ||
            (bank >= 0x80 && bank < 0xc0 && addr >= 0x8000) || bank >= 0xc0) {
            return cpu.memory.rom[hi_rom_resolve(addr)];
        }
    case EXHIROM:
        if ((bank < 0x40 && addr >= 0x8000) || (bank >= 0x40 && bank < 0x7d) ||
            (bank >= 0x80 && bank < 0xc0 && addr >= 0x8000) || (bank >= 0xc0)) {
                return cpu.memory.rom[ex_hi_rom_resolve(addr)];
            }
    }
    return 0;
}

void mmu_write(uint16_t addr, uint8_t bank, uint8_t value) {
    printf("%d%d%d", addr, bank, value);
}
