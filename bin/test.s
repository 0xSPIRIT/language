.intel_syntax noprefix
.global _start

_start:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 4], 50
  mov DWORD PTR [rbp - 8], 50
  mov eax, DWORD PTR [rbp - 4]
  sub eax, DWORD PTR [rbp - 8]
  cmp eax, 0
  sete al
  movzx eax, al
  test eax, eax
  je L0
  mov eax, DWORD PTR [rbp - 4]
  sub eax, DWORD PTR [rbp - 8]
  add rax, 1
  mov r11, rax
  mov rax, 2
  imul rax, r11
  jmp Lend_main
L0:
  mov eax, DWORD PTR [rbp - 4]
  jmp Lend_main
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 
