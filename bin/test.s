.intel_syntax noprefix
.global main
.type main, @function

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov DWORD PTR [rbp - 4], 6
  mov DWORD PTR [rbp - 8], 5
  mov r11d, DWORD PTR [rbp - 4]
  cmp r11d, DWORD PTR [rbp - 8]
  setne al
  movzx eax, al
  movzx BYTE PTR [rbp - 9], eax
  movzx eax, BYTE PTR [rbp - 9]
  jmp Lend_main
Lend_main:
  mov rsp, rbp
  pop rbp
  ret 
