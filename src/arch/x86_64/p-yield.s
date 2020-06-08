.weak liteco_internal_p_yield;
liteco_internal_p_yield = __p_yield;

.global __p_yield;
.align 2;
.type __p_yield, @function;
__p_yield:

    movq %rdi, %rax
again:
    PAUSE
    subq $1, %rax
    JNZ again
    ret
