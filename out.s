
.global main
main:
    push  %rbp               # save old stack frame base
    mov   %rsp, %rbp         # current top of stack is now bottom of new stack frame
    mov   $0,%rax            # move integer literal into %rax
    push  %rax               # save value of variable 'a' on the stack
    mov   $0,%rax            # move integer literal into %rax
    push  %rax               # save value of variable 'b' on the stack
    mov   -0(%rbp), %rax     # store variable 'b' value into %rax for use in an expression
    push  %rax               # store %rax for equal check
    mov   $0,%rax            # move integer literal into %rax
    pop   %rcx               # retrieve stored from stack
    cmp   %rax, %rcx         # perform equality check
    mov   $0,   %rax
    sete  %al
    cmp   $0,   %rax         # check if result was false for if statement
    je    _IfEndLabel0
    mov   -0(%rbp), %rax     # store variable 'a' value into %rax for use in an expression
    push  %rax               # store %rax for equal check
    mov   $0,%rax            # move integer literal into %rax
    pop   %rcx               # retrieve stored from stack
    cmp   %rax, %rcx         # perform equality check
    mov   $0,   %rax
    sete  %al
    cmp   $0,   %rax         # check if result was false for if statement
    je    _IfEndLabel0
    mov   $0,%rax            # move integer literal into %rax
    mov   %rax, -0(%rbp)     # store result into specified variable
    jmp   _IfEndLabel0
_IfEndLabel0:
    jmp   _IfEndLabel0
_IfEndLabel1:
    mov   -0(%rbp), %rax     # store variable 'a' value into %rax for use in an expression
    mov   %rbp, %rsp         # restore %rsp of caller
    pop   %rbp               # retore old %rbp
    ret
