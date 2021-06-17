
.global main
main:
mov   $100, %rax  # move integer literal into %rax
push  %rax        # store %rax for addition
mov   $2000, %rax # move integer literal into %rax
pop   %rcx        # retrieve stored from stack
add   %rcx, %rax  # add, store result in %rax
push  %rax        # store %rax for >= check
mov   $1001, %rax # move integer literal into %rax
pop   %rcx        # retrieve stored from stack
cmp   %eax, %ecx  # perform greater than eq check
mov   $0, %eax
setge %al
ret