#include "apu.h"
#include "cpu.h"
#include "ppu.h"
#include "spc.h"
#include "types.h"

extern cpu_t cpu;
extern spc_t spc;

#define get16bits(d)                                                           \
    ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) +                          \
     (uint32_t)(((const uint8_t *)(d))[0]))
// source: https://www.azillionmonkeys.com/qed/hash.html
uint32_t super_fast_hash(const char *data, uint32_t len) {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL)
        return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (; len > 0; len--) {
        hash += get16bits(data);
        tmp = (get16bits(data + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        data += 2 * sizeof(uint16_t);
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3:
        hash += get16bits(data);
        hash ^= hash << 16;
        hash ^= ((signed char)data[sizeof(uint16_t)]) << 18;
        hash += hash >> 11;
        break;
    case 2:
        hash += get16bits(data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1:
        hash += (signed char)*data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
#undef get16bits

static cart_hash_t rom_hash_lookup[] = {
    {"Super Mario World", 0x270efb15, LOROM},
    {"Super Mario All Stars", 0x2272b1cd, LOROM},
    {"SNES CPU Test", 0x69d6bf43, LOROM},
    {"SNES CPU Test Basic", 0x7b3f6de6, LOROM},
    {"SNES SPC Test", 0xb9a70b6a, LOROM},
    {"Earthbound", 0xb4975b60, HIROM},
    {"Puzzle Bobble", 0xc1a14c71, LOROM},
    {"Super Mario RPG", 0x834c11c2, LOROM},
    {"Super Mario Kart", 0x2202b32f, HIROM},
    {"Control Test Auto", 0xdd642b6a, LOROM},
    {"Control Test Simple", 0xc0467d5c, LOROM},
    {"Donkey Kong Country", 0xc1a8ad4c, HIROM}};

void at_exit(void) {
    for (uint16_t i = cpu.history_idx, j = spc.history_idx; i != cpu.history_idx - 1; i++, j++) {
        printf("0x%06x: 0x%02x\t 0x%04x: 0x%02x\n", cpu.pc_history[i],
               cpu.opcode_history[i], spc.pc_history[j], spc.opcode_history[j]);
    }
}

int main(int argc, char **argv) {
    ASSERT(argc == 2,
           "Incorrect parameter count: %d, expected 1, usage: ./snes <rom>.sfc",
           argc - 1);
    ASSERT(strncmp(".sfc", strrchr(argv[1], '.'), 5) == 0,
           "Incorrect file extension: %s, expected .sfc",
           strrchr(argv[1], '.'));

    cpu.file_name = argv[1];
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    uint32_t file_size = ftell(f);
    if (file_size % 1024 != 0) {
        fseek(f, 512, SEEK_SET);
    } else {
        fseek(f, 0, SEEK_SET);
    }

    // problem: SNES roms are in one of three possible layouts and it's not
    // documented in the rom which one it is
    // solution: hardcode hashes of the roms

    uint8_t *file_to_hash = calloc(file_size, 1);
    fread(file_to_hash, 1, file_size, f);
    fclose(f);
    uint32_t hash_value = super_fast_hash((char *)file_to_hash, file_size);
    log_message(LOG_LEVEL_INFO, "ROM hash value: 0x%x", hash_value);
    bool identified = false;
    memory_map_mode_t mode;
    for (uint32_t i = 0; i < ARRAYSIZE(rom_hash_lookup); i++) {
        if (rom_hash_lookup[i].hash == hash_value) {
            log_message(LOG_LEVEL_INFO, "Identified cart as %s",
                        rom_hash_lookup[i].name);
            identified = true;
            mode = rom_hash_lookup[i].mode;
            break;
        }
    }

    if (!identified) {
        ASSERT(0, "Cart not identified, hash value is 0x%x", hash_value);
    }

    uint8_t *header = NULL;
    switch (mode) {
    case LOROM:
        header = file_to_hash + 0x7fc0;
        break;
    case HIROM:
        header = file_to_hash + 0xffc0;
        break;
    case EXHIROM:
        header = file_to_hash + 0x40ffc0;
        break;
    default:
        UNREACHABLE_SWITCH(mode);
    }

    log_message(LOG_LEVEL_INFO, "Cartridge type: %d", header[0x16]);
    log_message(LOG_LEVEL_INFO, "Cart size: %d kilobytes; %d banks",
                file_size / 1024, file_size / 0x10000);
    log_message(LOG_LEVEL_INFO, "ROM size: %d kilobytes",
                (uint32_t)pow(2, header[0x17]));
    log_message(LOG_LEVEL_INFO, "RAM size: %d kilobytes",
                (uint32_t)pow(2, (double)(header[0x18])));

    cpu.memory.coprocessor = header[0x16] >> 8;
    cpu.memory.rom_size = pow(2, header[0x17]) * 1024;
    cpu.memory.rom = calloc(cpu.memory.rom_size, 1);
    if (header[0x16] == 0x00 || ((header[0x16] & 0xf) == 0x3) ||
        ((header[0x16] & 0xf) == 0x6)) {
        // since the size of the cart RAM is calculated with 1 << N, it is
        // impossible to specify for the ram to not exist using just this field.
        // Therefore, information is taken from the cart type to establish if
        // the cart should have RAM or not
        cpu.memory.sram_size = 0;
    } else {
        cpu.memory.sram_size = pow(2, header[0x18]) * 1024;
        cpu.memory.sram = calloc(cpu.memory.sram_size, 1);
        for (uint32_t i = 0; i < cpu.memory.sram_size; i++) {
            cpu.memory.sram[i] = 0xff;
        }
    }
    memcpy(cpu.memory.rom, file_to_hash, MIN(file_size, cpu.memory.rom_size));
    cpu.memory.mode = mode;
    cpu_reset();
    spc_reset();
    atexit(at_exit);
    apu_init();
    ui();
    apu_free();

    free(cpu.memory.sram);
    free(cpu.memory.rom);
}
