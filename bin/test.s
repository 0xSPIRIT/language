.intel_syntax noprefix

.section .rodata
.LS1: .asciz "%s\n"

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
  push rbx
  mov DWORD PTR [rbp - 8], edi
  mov QWORD PTR [rbp - 16], rsi
  mov DWORD PTR [rbp - 4], 0
L0:
  mov ebx, DWORD PTR [rbp - 4]
  cmp ebx, DWORD PTR [rbp - 8]
  setl al
  movzx eax, al
  test eax, eax
  je L1
  mov eax, DWORD PTR [rbp - 4]
  imul eax, 8
  movsx rbx, eax
  mov rax, QWORD PTR [rbp - 16]
  add rax, rbx
  mov rsi, QWORD PTR [rax]
  lea rdi, [rip + .LS1]
  xor al, al
  call printf
L2:
  mov eax, DWORD PTR [rbp - 4]
  add DWORD PTR [rbp - 4], 1
  jmp L0
L1:
  mov eax, 67
  jmp Lend_main
Lend_main:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 

.section .note.GNU-stack,"",@progbits
