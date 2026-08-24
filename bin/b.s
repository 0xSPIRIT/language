.intel_syntax noprefix

.section .text
.global main

nop:
  push rbp
  mov rbp, rsp
  sub rsp, 0
Lend_nop:
  mov rsp, rbp
  pop rbp
  ret 
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 4], edi
  mov QWORD PTR [rbp - 12], rsi
  mov eax, 67
  jmp Lend_main
Lend_main:
  mov rsp, rbp
  pop rbp
  ret 

.section .note.GNU-stack,"",@progbits
