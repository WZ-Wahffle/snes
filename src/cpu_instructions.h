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
OP(sed);
OP(cld);
OP(clv);
OP(xce);
OP(xba);
OP(rep);
OP(sep);
OP(tcd);
OP(tcs);
OP(tsc);
OP(tsb);
OP(trb);
OP(ldx);
OP(ldy);
OP(tya);
OP(tax);
OP(txa);
OP(tay);
OP(txy);
OP(tyx);
OP(txs);
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
OP(eor);
OP(bra);
OP(bmi);
OP(bpl);
OP(beq);
OP(bne);
OP(bcs);
OP(bcc);
OP(bvs);
OP(bvc);
OP(brl);
OP(jmp);
OP(jml);
OP(jsr);
OP(jsl);
OP(rts);
OP(rtl);
OP(rti);
OP(php);
OP(plp);
OP(pha);
OP(pla);
OP(phx);
OP(plx);
OP(phy);
OP(ply);
OP(phb);
OP(plb);
OP(phd);
OP(pld);
OP(phk);
OP(rol);
OP(ror);
OP(asl);
OP(lsr);
OP(bit);
OP(mvn);
OP(per);
OP(brk);
OP(cop);
#endif
