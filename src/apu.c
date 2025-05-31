#include "apu.h"
#include "raylib.h"
#include "types.h"

extern spc_t spc;

static void extract_sample(uint8_t brr, uint8_t shift, int16_t *out1,
                           int16_t *out2) {
    *out1 = (brr >> 4);
    if(*out1 > 7) *out1 -= 16;
    if (shift == 0)
        *out1 >>= 1;
    else
        *out1 <<= shift - 1;

    *out2 = (brr & 0xf);
    if(*out2 > 7) *out2 -= 16;
    if (shift == 0)
        *out2 >>= 1;
    else
        *out2 <<= shift - 1;
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

    uint16_t *out = buffer;
    for (uint32_t buffer_idx = 0; buffer_idx < count; buffer_idx++) {
        for (uint8_t channel_idx = 0; channel_idx < 8; channel_idx++) {
            dsp_channel_t *chan = &spc.memory.channels[channel_idx];
            if (!chan->playing && !chan->key_on) {
                continue;
            }
            if (chan->playing && chan->key_off) {
                chan->key_off = false;
                chan->playing = false;
                continue;
            }
            if (!chan->playing && chan->key_on) {
                chan->key_on = false;
                chan->playing = true;
                chan->t = 0;
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
                for (uint8_t i = 0; i < 6; i++) {
                    extract_sample(spc.memory.ram[chan->sample_addr++],
                                   chan->left_shift,
                                   &chan->sample_buffer[i * 2],
                                   &chan->sample_buffer[i * 2 + 1]);
                }
                chan->remaining_samples = 4;
            }

            chan->t += chan->pitch;
            if (chan->t >= 0xc000)
                chan->t -= 0xc000;

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
                chan->output = result;
            }
            // checking if 4 sample points have been passed
            if ((chan->t >> 12) % 4 == 0) {
                // load next 4 points
                uint8_t to = (chan->t >> 12) - 4;
                if (to > 12)
                    to += 12;
                for (uint8_t i = 0; i < 2; i++) {
                    extract_sample(spc.memory.ram[chan->sample_addr++],
                                   chan->left_shift,
                                   &chan->sample_buffer[to + i * 2],
                                   &chan->sample_buffer[to + i * 2 + 1]);
                }

                chan->remaining_samples -= 4;
            }
            if (chan->remaining_samples == 4 && chan->should_end) {
                chan->playing = false;
                continue;
            }
            if (chan->remaining_samples == 0) {
                chan->should_end = chan->end && !chan->loop;
                if (chan->end && chan->loop) {
                    chan->sample_addr = chan->loop_addr;
                }

                chan->left_shift = spc.memory.ram[chan->sample_addr] >> 4;
                chan->filter = (spc.memory.ram[chan->sample_addr] >> 2) & 0b11;
                chan->loop = spc.memory.ram[chan->sample_addr] & 0b10;
                chan->end = spc.memory.ram[chan->sample_addr++] & 0b1;
                chan->remaining_samples = 16;
            }
        }

        int16_t added_output = 0;
        for (uint8_t i = 0; i < 8; i++) {
            if (spc.memory.channels[i].playing)
                added_output += spc.memory.channels[i].output;
        }
        out[buffer_idx] = added_output;
    }
}

void apu_init(void) {
    SetTraceLogLevel(LOG_ERROR);
    InitAudioDevice();
    spc.memory.stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(spc.memory.stream, audio_cb);
    PlayAudioStream(spc.memory.stream);
}

void apu_free(void) {
    StopAudioStream(spc.memory.stream);
    UnloadAudioStream(spc.memory.stream);
    CloseAudioDevice();
}
