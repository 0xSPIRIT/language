.intel_syntax noprefix
.global _start

mul:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 4], edi
  mov DWORD PTR [rbp - 8], esi
  mov eax, DWORD PTR [rbp - 4]
  imul eax, DWORD PTR [rbp - 8]
  jmp Lend_mul
Lend_mul:
  mov rsp, rbp
  pop rbp
  ret 
add:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 4], edi
  mov DWORD PTR [rbp - 8], esi
  mov eax, DWORD PTR [rbp - 4]
  add eax, DWORD PTR [rbp - 8]
  jmp Lend_add
Lend_add:
  mov rsp, rbp
  pop rbp
  ret 
_start:
  push rbp
  mov rbp, rsp
  sub rsp, 0
  mov esi, 2
  mov edi, 5
  call add
  mov r10d, eax
  mov esi, 2
  mov edi, 2
  call mul
  mov r11d, eax
  mov eax, r10d
  add eax, r11d
  jmp Lend_main
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 
