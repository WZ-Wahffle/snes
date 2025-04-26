#include "cpu_mmu.h"

extern cpu_t cpu;
extern ppu_t ppu;
extern spc_t spc;

// https://snes.nesdev.org/wiki/Memory_map#LoROM
uint32_t lo_rom_resolve(uint32_t addr, bool log) {
    uint32_t ret = addr & 0x7fff;
    ret |= (addr >> 1) & 0b1111111000000000000000;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    if (log)
        log_message(LOG_LEVEL_VERBOSE,
                    "Resolved LoROM address 0x%06x to 0x%06x", addr, ret);
    return ret;
}

// https://snes.nesdev.org/wiki/Memory_map#HiROM
uint32_t hi_rom_resolve(uint32_t addr, bool log) {
    uint32_t ret = addr & 0x3fffff;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    if (log)
        log_message(LOG_LEVEL_VERBOSE,
                    "Resolved HiROM address 0x%06x to 0x%06x", addr, ret);
    return ret;
}

// https://snes.nesdev.org/wiki/Memory_map#ExHiROM
uint32_t ex_hi_rom_resolve(uint32_t addr, bool log) {
    uint32_t ret = addr & 0x3fffff;
    ret |= ((~addr) >> 1) & 0x400000;
    ASSERT(ret < cpu.memory.rom_size,
           "ROM index 0x%06x larger than size 0x%06x", ret,
           cpu.memory.rom_size);
    if (log)
        log_message(LOG_LEVEL_VERBOSE,
                    "Resolved ExHiROM address 0x%06x to 0x%06x", addr, ret);
    return ret;
}

uint8_t mmu_read(uint16_t addr, uint8_t bank, bool log) {
    // ROM resolution
    uint8_t ret;
    switch (cpu.memory.mode) {
    case LOROM:
        if ((bank <= 0x7d || bank >= 0x80) && addr >= 0x8000) {
            ret = cpu.memory.rom[lo_rom_resolve(TO_U24(addr, bank), log)];
            if (log)
                log_message(LOG_LEVEL_VERBOSE,
                            "Read 0x%02x from ROM address 0x%04x, bank 0x%02x",
                            ret, addr, bank);
            return ret;
        }
        break;
    case HIROM:
        if ((bank < 0x40 && addr >= 0x8000) ||
            (bank >= 0x80 && bank < 0xc0 && addr >= 0x8000) || bank >= 0xc0) {
            ret = cpu.memory.rom[hi_rom_resolve(TO_U24(addr, bank), log)];
            if (log)
                log_message(LOG_LEVEL_VERBOSE,
                            "Read 0x%02x from ROM address 0x%04x, bank 0x%02x",
                            ret, addr, bank);
            return ret;
        }
        break;
    case EXHIROM:
        if ((bank < 0x40 && addr >= 0x8000) || (bank >= 0x40 && bank < 0x7d) ||
            (bank >= 0x80 && bank < 0xc0 && addr >= 0x8000) || (bank >= 0xc0)) {
            ret = cpu.memory.rom[ex_hi_rom_resolve(TO_U24(addr, bank), log)];
            if (log)
                log_message(LOG_LEVEL_VERBOSE,
                            "Read 0x%02x from ROM address 0x%04x, bank 0x%02x",
                            ret, addr, bank);
            return ret;
        }
        break;
    }

    if ((bank < 0x40 || (bank >= 0x80 && bank < 0xc0)) && addr < 0x8000) {
        if (addr < 0x2000) {
            return cpu.memory.ram[addr];
        } else if (addr < 0x6000) {
            switch (addr) {
            case 0x2140:
            case 0x2141:
            case 0x2142:
            case 0x2143:
                return spc.memory.ram[0xf4 + (addr - 0x2140)];
                break;
            default:
                UNREACHABLE_SWITCH(addr);
            }
        }
    } else if (bank == 0x7e || bank == 0x7f) {
        return cpu.memory.ram[(bank - 0x7e) * 0x10000 + addr];
    }

    ASSERT(0, "Tried to read from bank 0x%02x, address 0x%04x", bank, addr);
}

void mmu_write(uint16_t addr, uint8_t bank, uint8_t value, bool log) {
    if ((bank < 0x40 || (bank >= 0x80 && bank < 0xc0)) && addr < 0x8000) {
        if (addr < 0x2000) {
            cpu.memory.ram[addr] = value;
        } else if (addr < 0x6000) {
            switch (addr) {
            case 0x2100:
                ppu.brightness = value & 0xf;
                ppu.force_blanking = value & 0x80;
                break;
            case 0x2101:
                ppu.obj_sprite_size = value >> 5;
                ppu.name_select = (value >> 3) & 0b11;
                ppu.name_base_address = value & 0b111;
                break;
            case 0x2115:
                ppu.address_increment_amount = value & 0b11;
                ppu.address_remapping = (value >> 2) & 0b11;
                ppu.address_increment_mode = value & 0x80;
                break;
            case 0x2116:
                ppu.vram_addr &= 0xff00;
                ppu.vram_addr |= value;
                break;
            case 0x2117:
                ppu.vram_addr &= 0xff;
                ppu.vram_addr |= value << 8;
                break;
            case 0x2140:
            case 0x2141:
            case 0x2142:
            case 0x2143:
                if (log)
                    log_message(LOG_LEVEL_INFO,
                                "CPU: wrote 0x%02x to port %d of APU bus",
                                value, addr - 0x2140 + 1);
                cpu.memory.apu_io[addr - 0x2140] = value;
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
            case 0x4300:
            case 0x4310:
            case 0x4320:
            case 0x4330:
            case 0x4340:
            case 0x4350:
            case 0x4360:
            case 0x4370:
                cpu.memory.dmas[(addr - 0x4300) / 16].direction = value & 0x80;
                cpu.memory.dmas[(addr - 0x4300) / 16].indirect_hdma =
                    value & 0x40;
                cpu.memory.dmas[(addr - 0x4300) / 16].addr_inc_mode =
                    (value >> 3) & 0b11;
                cpu.memory.dmas[(addr - 0x4300) / 16].transfer_pattern =
                    value & 0b111;
                break;
            case 0x4301:
            case 0x4311:
            case 0x4321:
            case 0x4331:
            case 0x4341:
            case 0x4351:
            case 0x4361:
            case 0x4371:
                cpu.memory.dmas[(addr - 0x4300) / 16].b_bus_addr = value;
                break;
            case 0x4302:
            case 0x4312:
            case 0x4322:
            case 0x4332:
            case 0x4342:
            case 0x4352:
            case 0x4362:
            case 0x4372:
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_src_addr &= 0xffff00;
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_src_addr |= value;
                break;
            case 0x4303:
            case 0x4313:
            case 0x4323:
            case 0x4333:
            case 0x4343:
            case 0x4353:
            case 0x4363:
            case 0x4373:
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_src_addr &= 0xff00ff;
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_src_addr |= value
                                                                      << 8;
                break;
            case 0x4304:
            case 0x4314:
            case 0x4324:
            case 0x4334:
            case 0x4344:
            case 0x4354:
            case 0x4364:
            case 0x4374:
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_src_addr &= 0xffff;
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_src_addr |= value
                                                                      << 16;
                break;
            case 0x4305:
            case 0x4315:
            case 0x4325:
            case 0x4335:
            case 0x4345:
            case 0x4355:
            case 0x4365:
            case 0x4375:
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_byte_count &=
                    0xffff00;
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_byte_count |= value;
                break;
            case 0x4306:
            case 0x4316:
            case 0x4326:
            case 0x4336:
            case 0x4346:
            case 0x4356:
            case 0x4366:
            case 0x4376:
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_byte_count &=
                    0xff00ff;
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_byte_count |= value
                                                                        << 8;
                break;
            case 0x4307:
            case 0x4317:
            case 0x4327:
            case 0x4337:
            case 0x4347:
            case 0x4357:
            case 0x4367:
            case 0x4377:
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_byte_count &= 0xffff;
                cpu.memory.dmas[(addr - 0x4300) / 16].dma_byte_count |= value
                                                                        << 16;
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
