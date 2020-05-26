#ifndef __LITECO_INTERNAL_CONTEXT_H__
#define __LITECO_INTERNAL_CONTEXT_H__

struct liteco_context {
    /**
     * SP(0)   | R8(8)   | R9(16)   | R10(24)   | R11(32)  | R12(40)   | R13(48) | R14(56) | R15(64) | RDI(72) | RSI(80)
     * RBP(88) | RBX(96) | RDX(104) | ARGS(112) | RCX(120) | FPTR(128) |
     */
    void *regs[32];
};

#endif
