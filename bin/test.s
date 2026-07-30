.intel_syntax noprefix
.global _start

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
mul:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 12], edi
  mov DWORD PTR [rbp - 16], esi
  mov DWORD PTR [rbp - 4], 0
  mov DWORD PTR [rbp - 8], 0
L0:
  mov r10d, DWORD PTR [rbp - 8]
  cmp r10d, DWORD PTR [rbp - 12]
  setl al
  movzx eax, al
  test eax, eax
  je L1
  mov esi, DWORD PTR [rbp - 16]
  mov edi, DWORD PTR [rbp - 4]
  call add
  mov DWORD PTR [rbp - 4], eax
  mov r10d, DWORD PTR [rbp - 8]
  mov eax, DWORD PTR [rbp - 8]
  add eax, 1
  mov DWORD PTR [rbp - 8], eax
  jmp L0
L1:
  mov eax, DWORD PTR [rbp - 4]
  jmp Lend_mul
Lend_mul:
  mov rsp, rbp
  pop rbp
  ret 
_start:
  push rbp
  mov rbp, rsp
  mov esi, 4
  mov edi, 2
  call mul
  jmp Lend_main
Lend_main:
  mov rdi, rax
  mov rax, 60
  syscall 
