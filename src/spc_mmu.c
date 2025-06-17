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
            return spc.memory.dsp_addr;
        case 0xf3: {
            switch (spc.memory.dsp_addr % 0x10) {
            case 0x00:
                return spc.memory.channels[spc.memory.dsp_addr / 16].vol_left;
            case 0x01:
                return spc.memory.channels[spc.memory.dsp_addr / 16].vol_right;
            case 0x02:
                return U16_LOBYTE(
                    spc.memory.channels[spc.memory.dsp_addr / 16].pitch);
            case 0x03:
                return U16_HIBYTE(
                    spc.memory.channels[spc.memory.dsp_addr / 16].pitch);
            case 0x04:
                return spc.memory.channels[spc.memory.dsp_addr / 16]
                    .sample_source_directory;
            case 0x05:
                return (spc.memory.channels[spc.memory.dsp_addr / 16]
                            .adsr_enable
                        << 7) |
                       (spc.memory.channels[spc.memory.dsp_addr / 16].d_rate
                        << 4) |
                       (spc.memory.channels[spc.memory.dsp_addr / 16].a_rate);
            case 0x06:
                return (spc.memory.channels[spc.memory.dsp_addr / 16].s_level
                        << 5) |
                       (spc.memory.channels[spc.memory.dsp_addr / 16].s_rate);
            case 0x07:
                return spc.memory.channels[spc.memory.dsp_addr / 16].gain;
            case 0x08:
                return spc.memory.channels[spc.memory.dsp_addr / 16].envx;
            case 0x09:
                return spc.memory.channels[spc.memory.dsp_addr / 16].outx;
            case 0x0c: {
                switch (spc.memory.dsp_addr) {
                case 0x0c:
                    return spc.memory.vol_left;
                case 0x1c:
                    return spc.memory.vol_right;
                case 0x2c:
                    return spc.memory.echo_left;
                case 0x3c:
                    return spc.memory.echo_right;
                case 0x4c:
                    return spc.memory.key_on;
                case 0x5c:
                    return spc.memory.key_off;
                case 0x6c:
                    return (spc.memory.mute_voices << 7) |
                           (spc.memory.mute_all << 6) |
                           (spc.memory.disable_echo_write << 5) |
                           spc.memory.noise_freq;
                case 0x7c:
                    return (spc.memory.channels[0].end << 0) |
                           (spc.memory.channels[1].end << 1) |
                           (spc.memory.channels[2].end << 2) |
                           (spc.memory.channels[3].end << 3) |
                           (spc.memory.channels[4].end << 4) |
                           (spc.memory.channels[5].end << 5) |
                           (spc.memory.channels[6].end << 6) |
                           (spc.memory.channels[7].end << 7);
                default:
                    UNREACHABLE_SWITCH(spc.memory.dsp_addr);
                }

                return 0;
            }
            case 0x0d:
                switch (spc.memory.dsp_addr) {
                case 0x0d:
                    return spc.memory.echo_feedback;
                case 0x7d:
                    return spc.memory.echo_delay;
                default:
                    UNREACHABLE_SWITCH(spc.memory.dsp_addr);
                }
            case 0x0f:
                return spc.memory.coefficients[spc.memory.dsp_addr / 16];
            default:
                UNREACHABLE_SWITCH(spc.memory.dsp_addr % 16);
            }
        }
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
                cpu.memory.apu_io[2] = 0;
                cpu.memory.apu_io[3] = 0;
            }
            if (val & 0x10) {
                cpu.memory.apu_io[0] = 0;
                cpu.memory.apu_io[1] = 0;
            }
            break;
        case 0xf2:
            spc.memory.dsp_addr = val;
            break;
        case 0xf3: {
            if (spc.memory.dsp_addr % 16 >= 9) {
                switch (spc.memory.dsp_addr) {
                case 0x0f:
                case 0x1f:
                case 0x2f:
                case 0x3f:
                case 0x4f:
                case 0x5f:
                case 0x6f:
                case 0x7f:
                    spc.memory.coefficients[spc.memory.dsp_addr / 16] = val;
                    break;
                case 0x0c:
                    spc.memory.vol_left = val;
                    break;
                case 0x1c:
                    spc.memory.vol_right = val;
                    break;
                case 0x2c:
                    spc.memory.echo_left = val;
                    break;
                case 0x3c:
                    spc.memory.echo_right = val;
                    break;
                case 0x4c:
                    spc.memory.key_on = val;
                    for (uint8_t i = 0; i < 8; i++) {
                        spc.memory.channels[i].key_on = val & (1 << i);
                    }
                    break;
                case 0x5c:
                    spc.memory.key_off = val;
                    for (uint8_t i = 0; i < 8; i++) {
                        spc.memory.channels[i].key_off = val & (1 << i);
                    }
                    break;
                case 0x6c:
                    spc.memory.mute_voices = val & 0x80;
                    if(spc.memory.mute_voices) {
                        for(uint8_t i = 0; i < 8; i++) {
                            spc.memory.channels[i].adsr_state = RELEASE;
                            spc.memory.channels[i].envelope = 0;
                        }
                    }
                    spc.memory.mute_all = val & 0x40;
                    spc.memory.disable_echo_write = val & 0x20;
                    spc.memory.noise_freq = val & 0x1f;
                    break;
                case 0x0d:
                    spc.memory.echo_feedback = val;
                    break;
                case 0x2d:
                    spc.memory.pitch_mod_enable = val;
                    break;
                case 0x3d:
                    spc.memory.use_noise = val;
                    break;
                case 0x4d:
                    spc.memory.echo_enable = val;
                    break;
                case 0x5d:
                    spc.memory.sample_source_directory_page = val;
                    break;
                case 0x6d:
                    spc.memory.echo_start_address = val;
                    break;
                case 0x7d:
                    spc.memory.echo_delay = val & 0xf;
                    break;
                default:
                    UNREACHABLE_SWITCH(spc.memory.dsp_addr);
                }
            } else {
                switch (spc.memory.dsp_addr % 16) {
                case 0:
                    spc.memory.channels[spc.memory.dsp_addr / 16].vol_left =
                        val;
                    break;
                case 1:
                    spc.memory.channels[spc.memory.dsp_addr / 16].vol_right =
                        val;
                    break;
                case 2:
                    spc.memory.channels[spc.memory.dsp_addr / 16].pitch &=
                        0xff00;
                    spc.memory.channels[spc.memory.dsp_addr / 16].pitch |= val;
                    break;
                case 3:
                    spc.memory.channels[spc.memory.dsp_addr / 16].pitch &= 0xff;
                    spc.memory.channels[spc.memory.dsp_addr / 16].pitch |= val
                                                                           << 8;
                    break;
                case 4:
                    spc.memory.channels[spc.memory.dsp_addr / 16]
                        .sample_source_directory = val;
                    break;
                case 5:
                    spc.memory.channels[spc.memory.dsp_addr / 16].adsr_enable =
                        val & 0x80;
                    spc.memory.channels[spc.memory.dsp_addr / 16].a_rate =
                        val & 0xf;
                    spc.memory.channels[spc.memory.dsp_addr / 16].d_rate =
                        (val >> 4) & 0b111;
                    break;
                case 6:
                    spc.memory.channels[spc.memory.dsp_addr / 16].s_level =
                        val >> 5;
                    spc.memory.channels[spc.memory.dsp_addr / 16].s_rate =
                        val & 0x1f;
                    break;
                case 7:
                    spc.memory.channels[spc.memory.dsp_addr / 16].gain = val;
                    break;
                case 8:
                    spc.memory.channels[spc.memory.dsp_addr / 16].envx = val;
                    break;
                case 9:
                    spc.memory.channels[spc.memory.dsp_addr / 16].outx = val;
                    break;
                default:
                    UNREACHABLE_SWITCH(spc.memory.dsp_addr % 16);
                }
            }
        } break;
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
