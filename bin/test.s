.intel_syntax noprefix

.section .text
.global main

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  mov DWORD PTR [rbp - 4], 0
  mov eax, DWORD PTR [rbp - 4]
  add eax, 1
  mov DWORD PTR [rbp - 4], eax
  cmp DWORD PTR [rbp - 4], 0
  setg al
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
  pop rbx
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 

.section .note.GNU-stack,"",@progbits
