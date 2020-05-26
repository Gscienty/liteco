.global __end_context;
.align 2;
.type __end_context, @function;
__end_context:
    ; /* 当协程执行完毕后，需执行在make context时铺设好的收尾工作 */
    ; /* 收尾工作需要现context中的link */
    ; /* 由于RSP指向的时当前协程的context，而context结构体的起始位置为link */
    ; /* 即当前RSP指向的内存空间内存储的值，即时link context的地址 */
    movq %rbx, %rsp
    movq (%rsp), %rdi
    testq %rdi, %rdi

    je no_linked_context

    call __set_context@plt
    movq %rax, %rdi

no_linked_context:
    call exit@plt

    hlt

.end __end_context;
.size __end_context,.-__end_context;
