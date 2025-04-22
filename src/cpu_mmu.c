#include "cpu_mmu.h"
#include "types.h"

extern cpu_t cpu;
extern ppu_t ppu;

// https://snes.nesdev.org/wiki/Memory_map#LoROM
static uint32_t lo_rom_resolve(uint32_t addr) {
    uint32_t ret = addr & 0x7fff;
    ret |= (addr >> 1) & 0b1111111000000000000000;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    log_message(LOG_LEVEL_VERBOSE, "Resolved LoROM address 0x%06x to 0x%06x",
                addr, ret);
    return ret;
}

// https://snes.nesdev.org/wiki/Memory_map#HiROM
static uint32_t hi_rom_resolve(uint32_t addr) {
    uint32_t ret = addr & 0x3fffff;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    log_message(LOG_LEVEL_VERBOSE, "Resolved HiROM address 0x%06x to 0x%06x",
                addr, ret);
    return ret;
}

// https://snes.nesdev.org/wiki/Memory_map#ExHiROM
static uint32_t ex_hi_rom_resolve(uint32_t addr) {
    uint32_t ret = addr & 0x3fffff;
    ret |= ((~addr) >> 1) & 0x400000;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    log_message(LOG_LEVEL_VERBOSE, "Resolved ExHiROM address 0x%06x to 0x%06x",
                addr, ret);
    return ret;
}

uint8_t mmu_read(uint16_t addr, uint8_t bank) {
    // ROM resolution
    uint8_t ret;
    switch (cpu.memory.mode) {
    case LOROM:
        if ((bank <= 0x7d || bank >= 0x80) && addr >= 0x8000) {
            ret = cpu.memory.rom[lo_rom_resolve(addr)];
            log_message(LOG_LEVEL_VERBOSE,
                        "Read 0x%02x from ROM address 0x%04x, bank 0x%02x", ret,
                        addr, bank);
            return ret;
        }
        break;
    case HIROM:
        if ((bank < 0x40 && addr >= 0x8000) ||
            (bank >= 0x80 && bank < 0xc0 && addr >= 0x8000) || bank >= 0xc0) {
            ret = cpu.memory.rom[hi_rom_resolve(addr)];
            log_message(LOG_LEVEL_VERBOSE,
                        "Read 0x%02x from ROM address 0x%04x, bank 0x%02x", ret,
                        addr, bank);
            return ret;
        }
        break;
    case EXHIROM:
        if ((bank < 0x40 && addr >= 0x8000) || (bank >= 0x40 && bank < 0x7d) ||
            (bank >= 0x80 && bank < 0xc0 && addr >= 0x8000) || (bank >= 0xc0)) {
            ret = cpu.memory.rom[ex_hi_rom_resolve(addr)];
            log_message(LOG_LEVEL_VERBOSE,
                        "Read 0x%02x from ROM address 0x%04x, bank 0x%02x", ret,
                        addr, bank);
            return ret;
        }
        break;
    }
    TODO("Reading from rest of memory");
}

void mmu_write(uint16_t addr, uint8_t bank, uint8_t value) {
    if ((bank < 0x40 || (bank >= 0x80 && bank < 0xc0)) && addr < 0x8000) {
        if (addr < 0x2000) {
            cpu.memory.ram[addr] = value;
        } else if (addr < 0x6000) {
            switch (addr) {
            case 0x2100:
                ppu.brightness = value & 0xf;
                ppu.force_blanking = value & 0x80;
                break;
            case 0x2140:
                cpu.memory.apu_bus[0] = value;
                break;
            case 0x2141:
                cpu.memory.apu_bus[1] = value;
                break;
            case 0x2142:
                cpu.memory.apu_bus[2] = value;
                break;
            case 0x2143:
                cpu.memory.apu_bus[3] = value;
                break;
            case 0x4200:
                cpu.vblank_nmi_enable = value & 0x80;
                cpu.timer_irq = (value >> 4) & 0b11;
                break;
            case 0x420b:
                for (uint8_t i = 0; i < 8; i++)
                    cpu.memory.dmas[i].dma_enable = value & (1 << i);
                break;
            case 0x420c:
                for (uint8_t i = 0; i < 8; i++)
                    cpu.memory.dmas[i].hdma_enable = value & (1 << i);
                break;
            default:
                UNREACHABLE_SWITCH(addr);
            }
        } else {
            TODO("expansion write");
        }
    } else if (bank == 0x7e || bank == 0x7f) {
        cpu.memory.ram[(bank - 0x7e) * 0x10000 + addr] = value;
    } else {
        ASSERT(0, "Tried to write 0x%02x to bank 0x%02x, address 0x%04x", value,
               bank, addr);
    }
}
