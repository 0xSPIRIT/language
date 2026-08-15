.intel_syntax noprefix

.section .rodata
.LS1: .asciz "ABCD"

.section .text
.global main

string_copy:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  mov QWORD PTR [rbp - 8], rdi
  mov QWORD PTR [rbp - 16], rsi
L0:
  mov rax, QWORD PTR [rbp - 16]
  mov bl, BYTE PTR [rax]
  test bl, bl
  je L1
  mov rax, QWORD PTR [rbp - 16]
  mov rax, QWORD PTR [rbp - 8]
  mov bl, BYTE PTR [rax]
  mov BYTE PTR [rax], bl
  mov rax, QWORD PTR [rbp - 16]
  mov rax, QWORD PTR [rbp - 16]
  add rax, 1
  mov QWORD PTR [rbp - 16], rax
  mov rax, QWORD PTR [rbp - 8]
  mov rax, QWORD PTR [rbp - 8]
  add rax, 1
  mov QWORD PTR [rbp - 8], rax
  jmp L0
L1:
  mov rax, QWORD PTR [rbp - 8]
  mov BYTE PTR [rax], 0
Lend_string_copy:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  lea rbx, [rip + .LS1]
  mov QWORD PTR [rbp - 8], rbx
  mov rdi, 100
  mov al, 0
  call malloc
  mov QWORD PTR [rbp - 16], rax
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
