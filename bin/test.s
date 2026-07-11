.intel_syntax noprefix
.global main
.type main, @function

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 4], 5
  mov DWORD PTR [rbp - 8], 5
  mov eax, DWORD PTR [rbp - 4]
  cmp eax, DWORD PTR [rbp - 8]
  setne al
  movzx rax, al
  test rax, rax
  je L0
  mov rax, 0
  jmp Lend_main
L0:
  mov rax, 1
  jmp Lend_main
Lend_main:
  mov rsp, rbp
  pop rbp
  ret 
