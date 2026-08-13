.intel_syntax noprefix

.section .rodata
.LS1: .asciz "%d "

.section .text
.global main

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  mov DWORD PTR [rbp - 4], 0
L0:
  cmp DWORD PTR [rbp - 4], 10
  setl al
  movzx eax, al
  test eax, eax
  je L1
  mov esi, DWORD PTR [rbp - 4]
  lea rdi, [rip + .LS1]
  mov al, 0
  call printf
L2:
  mov ebx, DWORD PTR [rbp - 4]
  mov eax, DWORD PTR [rbp - 4]
  add eax, 1
  mov DWORD PTR [rbp - 4], eax
  jmp L0
L1:
  mov eax, 0
  jmp Lend_main
  pop rbx
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 

.section .note.GNU-stack,"",@progbits
