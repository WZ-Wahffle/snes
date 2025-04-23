#ifndef INSTRUCTIONS_H_
#define INSTRUCTIONS_H_

#include "types.h"

#define OP(name) void name(addressing_mode_t mode)
#define LEGALADDRMODES(modes)                                                  \
    ASSERT((mode & (modes)) != 0,                                              \
           "Illegal address mode for mask: expected %d, found %s", modes,      \
           addressing_mode_strings[(uint8_t)log2(mode)])

OP(sei);
OP(stz);
OP(lda);
OP(sta);
OP(clc);
OP(xce);
OP(rep);
OP(sep);
OP(tcd);
OP(tcs);
OP(ldx);
OP(ldy);
OP(tya);
OP(tay);
OP(sec);
OP(sbc);
OP(dex);
OP(bpl);
#endif
