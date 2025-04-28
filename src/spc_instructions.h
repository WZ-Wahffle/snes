#ifndef SPC_INSTRUCTIONS_H_
#define SPC_INSTRUCTIONS_H_

#include "types.h"

#define OP(name) void spc_##name(spc_addressing_mode_t mode)
#define LEGALADDRMODES(modes)                                                  \
    ASSERT((mode & (modes)) != 0,                                              \
           "Illegal address mode for mask: expected %d, found %s", modes,      \
           addressing_mode_strings[(uint8_t)log2(mode)])

OP(lda);
OP(ldx);
OP(ldy);
OP(ldw);
OP(sta);
OP(stx);
OP(sty);
OP(stw);
OP(txs);
OP(txa);
OP(tax);
OP(tya);
OP(tay);
OP(inx);
OP(dex);
OP(iny);
OP(dey);
OP(bra);
OP(beq);
OP(bne);
OP(bmi);
OP(bpl);
OP(bcc);
OP(bbs0);
OP(bbs1);
OP(bbs2);
OP(bbs3);
OP(bbs4);
OP(bbs5);
OP(bbs6);
OP(bbs7);
OP(cbne);
OP(dbnz);
OP(jmp);
OP(jsr);
OP(rts);
OP(phy);
OP(ply);
OP(mov);
OP(cmp);
OP(cpx);
OP(cpy);
OP(inc);
OP(dew);
OP(adc);
OP(mul);
OP(div);
OP(asl);
OP(clp);
OP(clc);
OP(set0);
OP(set1);
OP(set2);
OP(set3);
OP(set4);
OP(set5);
OP(set6);
OP(set7);
OP(or1);
#endif
