.intel_syntax noprefix

.section .rodata
.LS1: .asciz "This is a first one!"
.LS2: .asciz "What"

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
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  mov rdi, 100
  call malloc
  mov QWORD PTR [rbp - 8], rax
  lea rsi, [rip + .LS1]
  mov rdi, QWORD PTR [rbp - 8]
  call strcpy
  mov rdi, QWORD PTR [rbp - 8]
  call puts
  mov rbx, QWORD PTR [rbp - 8]
  mov QWORD PTR [rbp - 16], rbx
  mov rdi, QWORD PTR [rbp - 16]
  call puts
  lea rsi, [rip + .LS2]
  mov rdi, QWORD PTR [rbp - 8]
  call strcpy
  mov rdi, QWORD PTR [rbp - 16]
  call puts
  mov eax, 67
  jmp Lend_main
  pop rbx
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 

.section .note.GNU-stack,"",@progbits
