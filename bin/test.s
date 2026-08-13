.intel_syntax noprefix

.section .rodata
.LS1: .asciz "Hello there matey!"

.section .text
.global main

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  push r12
  push r13
  lea rbx, QWORD PTR [rip + .LS1]
  mov QWORD PTR [rbp - 8], rbx
  mov DWORD PTR [rbp - 12], 0
L0:
  cmp DWORD PTR [rbp - 12], 10
  setl al
  movzx eax, al
  test eax, eax
  je L1
  mov rax, QWORD PTR [rbp - 8]
  movsx r12, DWORD PTR [rbp - 12]
  add rax, r12
  mov rdi, rax
  call puts
L2:
  mov r13d, DWORD PTR [rbp - 12]
  mov eax, DWORD PTR [rbp - 12]
  add eax, 1
  mov DWORD PTR [rbp - 12], eax
  jmp L0
L1:
  mov eax, 67
  jmp Lend_main
  pop r13
  pop r12
  pop rbx
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 

.section .note.GNU-stack,"",@progbits
