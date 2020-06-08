.weak liteco_internal_atomic_cas;
liteco_internal_atomic_cas = __atomic_cas;

.global __atomic_cas;
.align 2;
.type __atomic_cas, @function;
__atomic_cas:
    /*
        if ([$1] == $2) {
            [$1] = $3
        }
        return [$1] == $3;
     */

    mov %rdi, %rbx
    movq %rsi, %rax
    movq %rdx, %rcx

    lock
    cmpxchg %cx, 0(%rbx) 

    cmp 0(%rbx), %cx

    je seteq
    xorq %rax, %rax
    ret

seteq:
    movq $1, %rax
    ret

.end __atomic_cas;
.size __atomic_cas,.-__atomic_cas;
