.intel_syntax noprefix
.global _start

_start:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov QWORD PTR [rbp - 8], 1
  cmp QWORD PTR [rbp - 8], 0
  setl al
  movzx eax, al
  test eax, eax
  je L0
  mov eax, 67
  jmp Lend_main
  jmp L1
L0:
  mov eax, 69
  jmp Lend_main
L1:
  mov eax, DWORD PTR [rbp - 8]
  jmp Lend_main
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 
