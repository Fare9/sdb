.global main

.section .data

my_double: .double 64.125

.section .text

.macro trap
    # Trap
    movq $62, %rax # kill syscall
    movq %r12, %rdi # pid 1st parameter
    movq $5, %rsi # trap to do
    syscall
.endm

main:
    push %rbp
    movq %rsp, %rbp

    # Get PID
    movq $39, %rax
    syscall # Call for getPid
    movq %rax, %r12

    # Store to r13
    movq $0xcafecafe, %r13
    trap

    # Store to r13b
    movb $42, %r13b
    trap

    # Store to mm0
    movq $0xba5eba11, %r13
    movq %r13, %mm0
    trap

    # Store to xmm0
    movsd my_double(%rip), %xmm0
    trap

    # Store to st0
    emms # empty MMX technology state
    fldl my_double(%rip) # load floating-point value
    trap

    popq %rbp
    movq $0, %rax
    ret