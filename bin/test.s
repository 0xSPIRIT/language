.intel_syntax noprefix

.section .text
.global main

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov edi, 100
  call exit
  mov edi, -10
  call abs
  mov DWORD PTR [rbp - 4], eax
  mov eax, DWORD PTR [rbp - 4]
  jmp Lend_main
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 

.section .note.GNU-stack,"",@progbits
