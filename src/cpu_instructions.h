#ifndef CPU_INSTRUCTIONS_H_
#define CPU_INSTRUCTIONS_H_

#include "types.h"

#define OP(name) void name(addressing_mode_t mode)
#define LEGALADDRMODES(modes)                                                  \
    ASSERT((mode & (modes)) != 0,                                              \
           "Illegal address mode for mask: expected %d, found %s", modes,      \
           addressing_mode_strings[(uint8_t)log2(mode)])

OP(sei);
OP(cli);
OP(stz);
OP(lda);
OP(sta);
OP(clc);
OP(xce);
OP(xba);
OP(rep);
OP(sep);
OP(tcd);
OP(tcs);
OP(ldx);
OP(ldy);
OP(tya);
OP(tax);
OP(tay);
OP(sec);
OP(adc);
OP(sbc);
OP(cmp);
OP(cpx);
OP(inc);
OP(inx);
OP(dex);
OP(iny);
OP(dey);
OP(bra);
OP(bpl);
OP(beq);
OP(bne);
OP(bvs);
OP(jsr);
OP(rts);
OP(php);
OP(plp);
OP(pha);
OP(pla);
OP(rol);
#endif
