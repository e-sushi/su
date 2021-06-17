
.global main
main:
mov  $1, %rax
push %rax
mov  $1, %rax
pop  %rcx
add  %rcx, %rax
push %rax
mov  $1, %rax
pop  %rcx
add  %rcx, %rax
push %rax
mov  $1, %rax
pop  %rcx
add  %rcx, %rax
push %rax
mov  $1, %rax
pop  %rcx
add  %rcx, %rax
push %rax
mov  $1, %rax
pop  %rcx
add  %rcx, %rax
push %rax
mov  $1, %rax
pop  %rcx
add  %rcx, %rax
ret