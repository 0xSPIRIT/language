.intel_syntax noprefix

.section .rodata
.LS1: .asciz "Hello there"

.section .text
.global main

clrstr:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov QWORD PTR [rbp - 8], rdi
  mov rax, QWORD PTR [rbp - 8]
  mov BYTE PTR [rax], 0
Lend_clrstr:
  mov rsp, rbp
  pop rbp
  ret 
string_copy:
  push rbp
  mov rbp, rsp
  sub rsp, 32
  push rbx
  push r10
  push r11
  push r12
  push r13
  push r14
  mov QWORD PTR [rbp - 12], rdi
  mov QWORD PTR [rbp - 20], rsi
  mov DWORD PTR [rbp - 24], edx
  mov DWORD PTR [rbp - 4], 0
L0:
  mov ebx, DWORD PTR [rbp - 4]
  cmp ebx, DWORD PTR [rbp - 24]
  setl al
  movzx eax, al
  test eax, eax
  je L1
  mov rax, QWORD PTR [rbp - 20]
  movsx r10, DWORD PTR [rbp - 4]
  add rax, r10
  mov rax, QWORD PTR [rbp - 12]
  movsx r11, DWORD PTR [rbp - 4]
  add rax, r11
  lea r12, [rax]
  mov BYTE PTR [rax], r12b
L2:
  mov r13d, DWORD PTR [rbp - 4]
  mov eax, DWORD PTR [rbp - 4]
  add eax, 1
  mov DWORD PTR [rbp - 4], eax
  jmp L0
L1:
  mov rax, QWORD PTR [rbp - 12]
  movsx r14, DWORD PTR [rbp - 24]
  add rax, r14
  mov BYTE PTR [rax], 0
Lend_string_copy:
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 
main:
  push rbp
  mov rbp, rsp
  sub rsp, 24
  push rbx
  lea rbx, [rip + .LS1]
  mov QWORD PTR [rbp - 8], rbx
  mov rdi, 100
  mov al, 0
  call malloc
  mov QWORD PTR [rbp - 16], rax
  mov edx, 11
  mov rsi, QWORD PTR [rbp - 8]
  mov rdi, QWORD PTR [rbp - 16]
  mov al, 0
  call string_copy
  mov rdi, QWORD PTR [rbp - 8]
  mov al, 0
  call puts
  mov rdi, QWORD PTR [rbp - 16]
  mov al, 0
  call puts
  mov eax, 0
  jmp Lend_main
Lend_main:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 

.section .note.GNU-stack,"",@progbits
