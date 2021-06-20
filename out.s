

.global main

main:
    mov   $1000,%rax  # move integer literal into %rax
    push  %rax        # store %rax for bit or
    mov   $10,%rax    # move integer literal into %rax
    mov   %rax, %rcx  # mov %rax into %rcx for bitwise OR
    pop   %rax        # retrieve stored from stack
    or    %rcx, %rax  # bitwise and %rax with %rcx
    ret