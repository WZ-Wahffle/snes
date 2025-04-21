#ifndef INSTRUCTIONS_H_
#define INSTRUCTIONS_H_

#include "types.h"

void set_status_bit(status_bit_t bit, bool value);
bool get_status_bit(status_bit_t bit);
uint32_t resolve_addr(addressing_mode_t mode);

#define OP(name) void name(addressing_mode_t mode)
#define LEGALADDRMODES(modes)                                                  \
    ASSERT((mode & (modes)) != 0,                                              \
           "Illegal address mode for mask: expected %d, found %s", modes,      \
           addressing_mode_strings[(uint8_t)log2(mode)])

OP(sei);
OP(stz);
#endif
