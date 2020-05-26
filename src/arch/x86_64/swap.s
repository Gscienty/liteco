.weak liteco_swap_context;
liteco_swap_context = __swap_func;

.global __swap_func;
.align 2;
.type __swap_func, @function;
__swap_func:
    /* save current context */
    movq %r8,    8(%rdi)    ; // R8  save to context
    movq %r9,   16(%rdi)    ; // R9  save to context
    movq %r10,  24(%rdi)    ; // R10 save to context
    movq %r11,  32(%rdi)    ; // R11 save to context
    movq %r12,  40(%rdi)    ; // R12 save to context
    movq %r13,  48(%rdi)    ; // R13 save to context
    movq %r14,  56(%rdi)    ; // R14 save to context
    movq %r15,  64(%rdi)    ; // R15 save to context
    movq %rdi,  72(%rdi)    ; // RDI save to context
    movq %rsi,  80(%rdi)    ; // RSI save to context
    movq %rbp,  88(%rdi)    ; // RBP save to context
    movq %rbx,  96(%rdi)    ; // RBX save to context
    movq %rdx, 104(%rdi)    ; // RDX save to context
    movq $1,   112(%rdi)    ; // ARG save to context
    movq %rcx, 120(%rdi)    ; // RCX save to context

    movq (%rsp), %rcx
    movq %rcx, 128(%rdi)    ; // FPTR save to context

    leaq 8(%rsp), %rcx
    movq %rcx, (%rdi)       ; // SP save to context

    /* recover their context */
    movq    (%rsi) , %rsp       ; // RSP recover from context
    movq   8(%rsi) , %r8        ; // R8  recover from context
    movq  16(%rsi) , %r9        ; // R9  recover from context
    movq  24(%rsi) , %r10       ; // R10 recover from context
    movq  32(%rsi) , %r11       ; // R12 recover from context
    movq  40(%rsi) , %r12       ; // R13 recover from context
    movq  56(%rsi) , %r14       ; // R14 recover from context
    movq  64(%rsi) , %r15       ; // R15 recover from context
    movq  72(%rsi) , %rdi       ; // RDI recover from context
    movq  88(%rsi) , %rbp       ; // RBP recover from context
    movq  96(%rsi) , %rbx       ; // RBX recover from context
    movq  104(%rsi), %rdx       ; // RDX recover from context
    movq  112(%rsi), %rax       ; // RAX recover from context
    movq  120(%rsi), %rcx       ; // RCX recover from context

    pushq 120(%rsi)             ; // FPTR recover from context

    movq  80(%rsi) , %rsi       ; // RSI recover from context

    xorl %eax, %eax
    ret
.end __swap_func;
.size __swap_func,.-__swap_func;
