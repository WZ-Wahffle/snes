#include "apu.h"
#include "raylib.h"
#include "types.h"
#include <math.h>

extern cpu_t cpu;
extern spc_t spc;

static void extract_sample(uint8_t brr, dsp_channel_t *chan, int16_t *out1,
                           int16_t *out2) {
    *out1 = (brr >> 4);
    if (*out1 > 7)
        *out1 -= 16;
    if (chan->left_shift != 0)
        *out1 <<= chan->left_shift;
    *out1 >>= 1;

    *out2 = (brr & 0xf);
    if (*out2 > 7)
        *out2 -= 16;
    if (chan->left_shift != 0)
        *out2 <<= chan->left_shift;
    *out2 >>= 1;

    switch (chan->filter) {
    case 0:
        // this page intentionally left blank
        break;
    case 1:
        *out1 += chan->prev_sample * 0.9375f;
        *out2 += *out1 * 0.9375f;
        break;
    case 2:
        *out1 += chan->prev_sample * 1.90625f;
        *out1 -= chan->prev_prev_sample * 0.9375f;
        *out2 += *out1 * 1.90625f;
        *out2 -= chan->prev_sample * 0.9375f;
        break;
    case 3:
        *out1 += chan->prev_sample * 1.796875f;
        *out1 -= chan->prev_prev_sample * 0.8125f;
        *out2 += *out1 * 1.796875f;
        *out2 -= chan->prev_sample * 0.8125f;
        break;
    default:
        UNREACHABLE_SWITCH(chan->filter);
    }

    chan->prev_prev_sample = *out1;
    chan->prev_sample = *out2;
}

void audio_cb(void *buffer, unsigned int count) {
    static const uint16_t gauss_lut[512] = {
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    3,    3,
        3,    3,    3,    4,    4,    4,    4,    4,    5,    5,    5,    5,
        6,    6,    6,    6,    7,    7,    7,    8,    8,    8,    9,    9,
        9,    10,   10,   10,   11,   11,   11,   12,   12,   13,   13,   14,
        14,   15,   15,   15,   16,   16,   17,   17,   18,   19,   19,   20,
        20,   21,   21,   22,   23,   23,   24,   24,   25,   26,   27,   27,
        28,   29,   29,   30,   31,   32,   32,   33,   34,   35,   36,   36,
        37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
        49,   50,   51,   52,   53,   54,   55,   56,   58,   59,   60,   61,
        62,   64,   65,   66,   67,   69,   70,   71,   73,   74,   76,   77,
        78,   80,   81,   83,   84,   86,   87,   89,   90,   92,   94,   95,
        97,   99,   100,  102,  104,  106,  107,  109,  111,  113,  115,  117,
        118,  120,  122,  124,  126,  128,  130,  132,  134,  137,  139,  141,
        143,  145,  147,  150,  152,  154,  156,  159,  161,  163,  166,  168,
        171,  173,  175,  178,  180,  183,  186,  188,  191,  193,  196,  199,
        201,  204,  207,  210,  212,  215,  218,  221,  224,  227,  230,  233,
        236,  239,  242,  245,  248,  251,  254,  257,  260,  263,  267,  270,
        273,  276,  280,  283,  286,  290,  293,  297,  300,  304,  307,  311,
        314,  318,  321,  325,  328,  332,  336,  339,  343,  347,  351,  354,
        358,  362,  366,  370,  374,  378,  381,  385,  389,  393,  397,  401,
        405,  410,  414,  418,  422,  426,  430,  434,  439,  443,  447,  451,
        456,  460,  464,  469,  473,  477,  482,  486,  491,  495,  499,  504,
        508,  513,  517,  522,  527,  531,  536,  540,  545,  550,  554,  559,
        563,  568,  573,  577,  582,  587,  592,  596,  601,  606,  611,  615,
        620,  625,  630,  635,  640,  644,  649,  654,  659,  664,  669,  674,
        678,  683,  688,  693,  698,  703,  708,  713,  718,  723,  728,  732,
        737,  742,  747,  752,  757,  762,  767,  772,  777,  782,  787,  792,
        797,  802,  806,  811,  816,  821,  826,  831,  836,  841,  846,  851,
        855,  860,  865,  870,  875,  880,  884,  889,  894,  899,  904,  908,
        913,  918,  923,  927,  932,  937,  941,  946,  951,  955,  960,  965,
        969,  974,  978,  983,  988,  992,  997,  1001, 1005, 1010, 1014, 1019,
        1023, 1027, 1032, 1036, 1040, 1045, 1049, 1053, 1057, 1061, 1066, 1070,
        1074, 1078, 1082, 1086, 1090, 1094, 1098, 1102, 1106, 1109, 1113, 1117,
        1121, 1125, 1128, 1132, 1136, 1139, 1143, 1146, 1150, 1153, 1157, 1160,
        1164, 1167, 1170, 1174, 1177, 1180, 1183, 1186, 1190, 1193, 1196, 1199,
        1202, 1205, 1207, 1210, 1213, 1216, 1219, 1221, 1224, 1227, 1229, 1232,
        1234, 1237, 1239, 1241, 1244, 1246, 1248, 1251, 1253, 1255, 1257, 1259,
        1261, 1263, 1265, 1267, 1269, 1270, 1272, 1274, 1275, 1277, 1279, 1280,
        1282, 1283, 1284, 1286, 1287, 1288, 1290, 1291, 1292, 1293, 1294, 1295,
        1296, 1297, 1297, 1298, 1299, 1300, 1300, 1301, 1302, 1302, 1303, 1303,
        1303, 1304, 1304, 1304, 1304, 1304, 1305, 1305,
    };

    static const float noise_generator_frequencies[] = {
        INFINITY,      1.f / 16.f,    1.f / 21.f,   1.f / 25.f,   1.f / 31.f,
        1.f / 42.f,    1.f / 50.f,    1.f / 63.f,   1.f / 83.f,   1.f / 100.f,
        1.f / 125.f,   1.f / 167.f,   1.f / 200.f,  1.f / 250.f,  1.f / 333.f,
        1.f / 400.f,   1.f / 500.f,   1.f / 667.f,  1.f / 800.f,  1.f / 1000.f,
        1.f / 1300.f,  1.f / 1600.f,  1.f / 2000.f, 1.f / 2700.f, 1.f / 3200.f,
        1.f / 4000.f,  1.f / 5300.f,  1.f / 6400.f, 1.f / 8000.f, 1.f / 10700.f,
        1.f / 16000.f, 1.f / 32000.f,
    };

    uint16_t *out = buffer;
    static uint16_t smp_counter = 0;
    static uint16_t noise_generator = 0x4000;
    static float noise_generator_timer = 0;
    for (uint32_t buffer_idx = 0; buffer_idx < count; buffer_idx++) {
        if (spc.memory.noise_freq != 0) {
            noise_generator_timer += 1.f / SAMPLE_RATE;
            while (noise_generator_timer >
                   noise_generator_frequencies[spc.memory.noise_freq]) {
                noise_generator =
                    (noise_generator >> 1) |
                    (((noise_generator << 14) ^ (noise_generator << 13)) &
                     0x4000);
                noise_generator_timer -=
                    noise_generator_frequencies[spc.memory.noise_freq];
            }
        }
        for (uint8_t channel_idx = 0; channel_idx < 8; channel_idx++) {
            dsp_channel_t *chan = &spc.memory.channels[channel_idx];
            if (!chan->playing && !chan->key_on) {
                continue;
            }
            if (chan->key_off) {
                chan->key_off = false;
                spc.memory.key_off &= ~(1 << channel_idx);
                chan->adsr_state = RELEASE;
                continue;
            }
            if (chan->key_on) {
                // channel is turned on
                chan->key_on = false;
                spc.memory.key_on &= ~(1 << channel_idx);
                chan->envelope = 0;
                chan->adsr_state = ATTACK;
                chan->playing = true;
                chan->t = 0;
                // reload pointers
                uint16_t addr = (spc.memory.sample_source_directory_page << 8) +
                                (chan->sample_source_directory << 2);
                chan->sample_addr =
                    TO_U16(spc.memory.ram[addr], spc.memory.ram[addr + 1]);
                chan->loop_addr =
                    TO_U16(spc.memory.ram[addr + 2], spc.memory.ram[addr + 3]);
                chan->left_shift = spc.memory.ram[chan->sample_addr] >> 4;
                chan->filter = (spc.memory.ram[chan->sample_addr] >> 2) & 0b11;
                chan->loop = spc.memory.ram[chan->sample_addr] & 0b10;
                chan->end = spc.memory.ram[chan->sample_addr++] & 0b1;
                // preload first 12 sample points (of 16) from first sample
                for (uint8_t i = 0; i < 6; i++) {
                    extract_sample(spc.memory.ram[chan->sample_addr++], chan,
                                   &chan->sample_buffer[i * 2],
                                   &chan->sample_buffer[i * 2 + 1]);
                }
                chan->remaining_values_in_block = 4;
                chan->refill_idx = 0;
            }

            static const uint16_t period[] = {
                65535, 2048, 1536, 1280, 1024, 768, 640, 512, 384, 320, 256,
                192,   160,  128,  96,   80,   64,  48,  40,  32,  24,  20,
                16,    12,   10,   8,    6,    5,   4,   3,   2,   1};
            static const uint16_t offset[] = {536, 0, 1040};

            if (chan->adsr_enable) {
                switch (chan->adsr_state) {
                case ATTACK:
                    if ((smp_counter + offset[(chan->a_rate * 2 + 1) % 3]) %
                            period[chan->a_rate * 2 + 1] ==
                        0) {
                        chan->envelope += chan->a_rate == 0xf ? 1024 : 32;
                        if (chan->envelope >= 0x7ff) {
                            chan->envelope = 0x7ff;
                            chan->adsr_state = DECAY;
                        }
                    }
                    break;
                case DECAY:
                    if ((smp_counter + offset[(chan->d_rate * 2 + 16) % 3]) %
                            period[chan->d_rate * 2 + 16] ==
                        0) {
                        chan->envelope -= ((chan->envelope - 1) >> 8) + 1;
                        if (chan->envelope <= (chan->s_level + 1) * 256) {
                            chan->envelope = (chan->s_level + 1) * 256;
                            chan->adsr_state = SUSTAIN;
                        }
                    }
                    break;
                case SUSTAIN:
                    if (chan->s_rate != 0 &&
                        (smp_counter + offset[chan->s_rate % 3]) %
                                period[chan->s_rate] ==
                            0) {
                        chan->envelope -= ((chan->envelope - 1) >> 8) + 1;
                        if (chan->envelope < 0) {
                            chan->envelope = 0;
                        }
                    }
                    break;
                case RELEASE:
                    chan->envelope -= 8;
                    if (chan->envelope < 0) {
                        chan->envelope = 0;
                        chan->playing = false;
                    }
                    break;
                }
            } else {
                if (chan->gain & 0x80) {
                    // direct
                    chan->envelope = (chan->gain & 0x7f) << 4;
                } else {
                    uint8_t gain_value = chan->gain & 0x1f;
                    if (gain_value != 0 &&
                        (smp_counter + offset[gain_value % 3]) %
                                period[gain_value] ==
                            0) {
                        switch ((chan->gain >> 5) & 0b11) {
                        case 0:
                            chan->envelope -= 32;
                            if (chan->envelope < 0)
                                chan->envelope = 0;
                            break;
                        case 1:
                            chan->envelope -= 1;
                            chan->envelope -= chan->envelope >> 8;
                            if (chan->envelope < 0)
                                chan->envelope = 0;
                            break;
                        case 2:
                            chan->envelope += 32;
                            if (chan->envelope > 0x7ff)
                                chan->envelope = 0x7ff;
                            break;
                        case 3:
                            if (chan->envelope < 0x600) {
                                chan->envelope += 32;
                            } else {
                                chan->envelope += 8;
                            }
                            if (chan->envelope > 0x7ff)
                                chan->envelope = 0x7ff;
                            break;
                        default:
                            UNREACHABLE_SWITCH((chan->gain >> 5) & 0b11);
                        }
                    }
                }
            }

            chan->t += chan->pitch;
            if (chan->t >= 0xc000)
                chan->t -= 0xc000;
            chan->points_passed_since_refill += chan->pitch / 4096.f;

            if (chan->playing) {
                // play sample
                uint8_t idx = chan->t >> 12;
                uint16_t table_idx = (chan->t >> 4) & 0xff;
                uint32_t result = 0;
                result += gauss_lut[255 - table_idx] * chan->sample_buffer[idx];
                result += gauss_lut[511 - table_idx] *
                          chan->sample_buffer[(idx + 1) % 12];
                result += gauss_lut[256 + table_idx] *
                          chan->sample_buffer[(idx + 2) % 12];
                result +=
                    gauss_lut[table_idx] * chan->sample_buffer[(idx + 3) % 12];
                result >>= 11;

                if (spc.memory.use_noise & (1 << channel_idx)) {
                    chan->output = noise_generator;
                } else {
                    chan->output = result;
                }
            }
            // checking if 4 sample points have been passed
            if (chan->points_passed_since_refill >= 4) {
                if (chan->should_end) {
                    chan->points_passed_since_refill -= 4;
                    chan->clear_out_count -= 4;
                    if (chan->clear_out_count == 0) {
                        chan->playing = false;
                    }
                }
                // load next 4 points
                for (uint8_t i = 0; i < 2; i++) {
                    extract_sample(
                        spc.memory.ram[chan->sample_addr++], chan,
                        &chan->sample_buffer[chan->refill_idx + i * 2],
                        &chan->sample_buffer[chan->refill_idx + i * 2 + 1]);
                }

                chan->refill_idx += 4;
                chan->refill_idx %= 12;
                chan->points_passed_since_refill -= 4;
                chan->remaining_values_in_block -= 4;
            }
            if (chan->remaining_values_in_block == 0) {
                chan->should_end = chan->end && !chan->loop;

                if (chan->should_end) {
                    chan->clear_out_count = 12;
                } else {
                    if (chan->end && chan->loop) {
                        chan->sample_addr = chan->loop_addr;
                    }

                    chan->left_shift = spc.memory.ram[chan->sample_addr] >> 4;
                    chan->filter =
                        (spc.memory.ram[chan->sample_addr] >> 2) & 0b11;
                    chan->loop = spc.memory.ram[chan->sample_addr] & 0b10;
                    chan->end = spc.memory.ram[chan->sample_addr++] & 0b1;
                    chan->remaining_values_in_block = 16;
                }
            }
        }

        int16_t added_output_left = 0;
        int16_t added_output_right = 0;
        for (uint8_t i = 0; i < 8; i++) {
            spc.memory.channels[i].outx = spc.memory.channels[i].output / 256;
            spc.memory.channels[i].envx = spc.memory.channels[i].envelope / 16;
            if (spc.memory.channels[i].playing &&
                !spc.memory.channels[i].mute_override) {
                added_output_left += spc.memory.channels[i].output *
                                     (spc.memory.channels[i].vol_left / 128.f) *
                                     (spc.memory.channels[i].envelope / 2048.f);
                added_output_right +=
                    spc.memory.channels[i].output *
                    (spc.memory.channels[i].vol_right / 128.f) *
                    (spc.memory.channels[i].envelope / 2048.f);
            }
        }
        out[buffer_idx * 2] = added_output_left;
        out[buffer_idx * 2 + 1] = added_output_right;
        if (smp_counter == 0)
            smp_counter = 30720;
        else
            smp_counter--;
    }
}

void apu_init(void) {
    SetTraceLogLevel(LOG_ERROR);
    InitAudioDevice();
    spc.memory.stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(spc.memory.stream, audio_cb);
    PlayAudioStream(spc.memory.stream);
}

void apu_free(void) {
    StopAudioStream(spc.memory.stream);
    UnloadAudioStream(spc.memory.stream);
    CloseAudioDevice();
}
