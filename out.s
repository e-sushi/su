
.global main
main:
    push  %rbp               # save old stack frame base
    mov   %rsp, %rbp         # current top of stack is now bottom of new stack frame
    mov   $1,%rax            # move integer literal into %rax
    push  %rax               # store %rax for subtraction
    mov   $2,%rax            # move integer literal into %rax
    push  %rax               # store %rax for multiplication
    mov   $3,%rax            # move integer literal into %rax
    pop   %rcx               # retrieve stored from stack
    imul  %rcx, %rax         # signed multiply, store result in %rax
    mov   %rax, %rcx         # mov %rax into %rcx for subtraction
    pop   %rax               # retrieve stored from stack
    sub   %rcx, %rax         # sub, store result in %rax
    push  %rax               # store %rax for addition
    mov   $4,%rax            # move integer literal into %rax
    pop   %rcx               # retrieve stored from stack
    add   %rcx, %rax         # add, store result in %rax
    mov   %rbp, %rsp         # restore %rsp of caller
    pop   %rbp               # retore old %rbp
    ret
