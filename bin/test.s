.intel_syntax noprefix

.section .rodata
.LS1: .asciz "We got %d arguments!\n"
.LS2: .asciz "%d "

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
  sub rsp, 32
  push rbx
  mov DWORD PTR [rbp - 24], edi
  mov QWORD PTR [rbp - 32], rsi
  mov eax, DWORD PTR [rbp - 24]
  sub eax, 1
  mov DWORD PTR [rbp - 4], eax
  mov eax, DWORD PTR [rbp - 4]
  imul eax, 4
  mov edi, eax
  call malloc
  mov QWORD PTR [rbp - 12], rax
  mov esi, DWORD PTR [rbp - 4]
  lea rdi, [rip + .LS1]
  xor al, al
  call printf
  mov DWORD PTR [rbp - 16], 1
L0:
  mov ebx, DWORD PTR [rbp - 16]
  cmp ebx, DWORD PTR [rbp - 24]
  setl al
  movzx eax, al
  test eax, eax
  je L1
  mov eax, DWORD PTR [rbp - 16]
  imul eax, 8
  movsx rbx, eax
  mov rax, QWORD PTR [rbp - 32]
  add rax, rbx
  mov rdi, QWORD PTR [rax]
  call atoi
  mov eax, DWORD PTR [rbp - 16]
  sub eax, 1
  mov eax, DWORD PTR [rbp - 16]
  sub eax, 1
  movsx rbx, eax
  mov rax, QWORD PTR [rbp - 12]
  lea rax, [rax + rbx*4]
  mov DWORD PTR [rax], eax
L2:
  mov eax, DWORD PTR [rbp - 16]
  add DWORD PTR [rbp - 16], 1
  jmp L0
L1:
  mov DWORD PTR [rbp - 20], 0
L3:
  mov ebx, DWORD PTR [rbp - 20]
  cmp ebx, DWORD PTR [rbp - 4]
  setl al
  movzx eax, al
  test eax, eax
  je L4
  mov eax, DWORD PTR [rbp - 20]
  imul eax, 4
  movsx rbx, eax
  mov rax, QWORD PTR [rbp - 12]
  add rax, rbx
  mov esi, DWORD PTR [rax]
  lea rdi, [rip + .LS2]
  xor al, al
  call printf
L5:
  mov eax, DWORD PTR [rbp - 20]
  add DWORD PTR [rbp - 20], 1
  jmp L3
L4:
  mov eax, 67
  jmp Lend_main
Lend_main:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 

.section .note.GNU-stack,"",@progbits
