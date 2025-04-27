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
OP(stx);
OP(sty);
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
OP(txa);
OP(tay);
OP(txy);
OP(tyx);
OP(sec);
OP(adc);
OP(sbc);
OP(cmp);
OP(cpx);
OP(cpy);
OP(inc);
OP(dec);
OP(inx);
OP(dex);
OP(iny);
OP(dey);
OP(and_);
OP(ora);
OP(bra);
OP(bmi);
OP(bpl);
OP(beq);
OP(bne);
OP(bcs);
OP(bcc);
OP(bvs);
OP(jmp);
OP(jml);
OP(jsr);
OP(jsl);
OP(rts);
OP(rtl);
OP(php);
OP(plp);
OP(pha);
OP(pla);
OP(phy);
OP(ply);
OP(phb);
OP(plb);
OP(phk);
OP(rol);
OP(asl);
#endif
