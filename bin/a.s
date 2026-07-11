.intel_syntax noprefix
.globl _start

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16

  mov DWORD PTR [rbp-4], 5
  mov DWORD PTR [rbp-4], 2
  mov DWORD PTR [rbp-8], 65
  mov DWORD PTR [rbp-8], 3
  mov eax, DWORD PTR [rbp-8]
  mov DWORD PTR [rbp-4], eax

  mov rsp, rbp
  pop rbp
  ret

_start:
  call main

  mov edi, eax
  mov eax, 60
  syscall
