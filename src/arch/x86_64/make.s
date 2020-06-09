.weak liteco_internal_context_make;
liteco_internal_context_make = __make_context;

.global __make_context;
.align 2;
.type __make_context, @function;
__make_context:
/* rdi (ctx), rsi (stack), rdx (size), rcx(func), r8(arg) */

    movq %rcx, 136(%rdi)                ; /* 将函数指针存入到context的起始函数的位置 */

    movq %rsi, %rax                     ; /* 设置栈，将栈空间的基址存入rax寄存器中 */
    addq %rdx, %rax                     ; /* 计算出栈顶指针所指向的内存地址 */

                                        ; /* 对齐栈顶指针 */
    subq $8, %rax
    andq $0xfffffffffffffff0, %rax
    subq $8, %rax

    addq $8, %rax
    movq %rax, 96(%rdi)                 ; /* 将该上下文的基址地址存储到context的RBX的位置 */
    subq $8, %rax
    movq %rax, 128(%rdi)                ; /* 将该上下文的栈顶指针存储到context的RSP的位置 */
    movq (%rdi), %r9
    movq %r9, 8(%rax)                   ; /* 将关联context存入到协程栈栈底 */
                                        ; /* 此时假设SP为0，则内存分布如下： */
                                        ; /* SP(0): stored __end_context, RBX(8) stored context */

    movq %r8, 72(%rdi)                  ; /* 将arg存入context中的RDI的位置 */

    xorl %eax, %eax
    ret
.end __make_context;
.size __make_context,.-__make_context;
