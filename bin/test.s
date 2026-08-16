.intel_syntax noprefix

.section .rodata
.LS1: .asciz "ABCD"

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
  lea rbx, [rip + .LS1]
  mov QWORD PTR [rbp - 8], rbx
  mov rdi, 50
  mov al, 0
  call malloc
  mov QWORD PTR [rbp - 16], rax
  mov rsi, QWORD PTR [rbp - 8]
  mov rdi, QWORD PTR [rbp - 16]
  mov al, 0
  call strcpy
  mov al, 0
  call nop
  mov rax, QWORD PTR [rbp - 8]
  lea rax, [rax + 1]
  mov rax, QWORD PTR [rbp - 16]
  mov bl, BYTE PTR [rax]
  mov BYTE PTR [rax], bl
  mov al, 0
  call nop
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
