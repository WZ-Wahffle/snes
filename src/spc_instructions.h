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
OP(sty);
OP(stw);
OP(txs);
OP(tax);
OP(tya);
OP(inx);
OP(dex);
OP(iny);
OP(dey);
OP(bra);
OP(bne);
OP(bpl);
OP(jmp);
OP(mov);
OP(cmp);
OP(cpy);
OP(inc);
#endif
