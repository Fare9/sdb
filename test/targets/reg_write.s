.global main

.section .data

hex_format: .asciz "%#x"
float_format: .asciz "%.2f"
long_float_format: .asciz "%.2Lf"

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
    trap

    # Print contents of rsi
    leaq hex_format(%rip), %rdi # load effective address using pc-relative addressing to rdi
    movq $0, %rax
    call printf@plt # call to printf in the plt
    movq $0, %rdi
    call fflush@plt # flush buffer
    trap

    # Now we will print contents of mm0
    movq %mm0, %rsi
    leaq hex_format(%rip), %rdi
    movq $0, %rax
    call printf@plt
    movq $0, %rdi
    call fflush@plt
    trap

    # print contents of xmm0
    leaq float_format(%rip), %rdi
    movq $1, %rax # with this we tell printf that there is a vector argument
    call printf@plt # print the vector directly from xmm0
    movq $0, %rdi
    call fflush@plt
    trap

    # Print contents of st0 (an x87 register)
    subq $16, %rsp # get space for register
    fstpt (%rsp)   # store the st0 register from fpu stack into rsp
    leaq long_float_format(%rip), %rdi
    movq $0, %rax
    call printf@plt # print float value
    movq $0, %rdi
    call fflush@plt
    addq $16, %rsp # clean the stack
    trap

    popq %rbp
    movq $0, %rax
    ret