  .text                    # start of code section
  .globl   main            # marks the function 'main' as being global
main:
  push     %rbp            # save base pointer to stack (start scope)
  mov      %rsp,%rbp       # put the previous stack pointer into the base pointer
  mov      $2,rax          # move integer literal into %rax
  leave                    # undo stack pointer move and push (end scope)
  ret                      # return code pointer back to func call site
