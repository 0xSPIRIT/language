.intel_syntax noprefix

.section .rodata
.LS1: .asciz "ABCD"

.section .text
.global main

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  lea rbx, [rip + .LS1]
  mov QWORD PTR [rbp - 8], rbx
  mov eax, 3
  sub eax, 1
  movsx rbx, eax
  mov rax, QWORD PTR [rbp - 8]
  lea rax, [rax + rbx*1]
  mov bl, BYTE PTR [rax]
  mov BYTE PTR [rbp - 9], bl
  movzx edi, BYTE PTR [rbp - 9]
  mov al, 0
  call putchar
  mov edi, 10
  mov al, 0
  call putchar
  mov eax, 0
  jmp Lend_main
Lend_main:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 

.section .note.GNU-stack,"",@progbits
