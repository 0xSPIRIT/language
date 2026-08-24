.intel_syntax noprefix

.section .rodata
.LS1: .asciz " %[^\n]"
.LS2: .asciz "Usage: %s <a> <b>\n"
.LS3: .asciz "Either a or b is nonzero."
.LS4: .asciz "Both a and b are zero."
.LS5: .asciz "I got my value as %d\n"

.section .text
.global main

read_line_into:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov QWORD PTR [rbp - 8], rdi
  mov rsi, QWORD PTR [rbp - 8]
  lea rdi, [rip + .LS1]
  xor al, al
  call scanf
Lend_read_line_into:
  mov rsp, rbp
  pop rbp
  ret 
string_copy:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  push r10
  mov QWORD PTR [rbp - 8], rdi
  mov QWORD PTR [rbp - 16], rsi
L0:
  mov rax, QWORD PTR [rbp - 16]
  mov bl, BYTE PTR [rax]
  test bl, bl
  je L1
  mov rax, QWORD PTR [rbp - 16]
  add QWORD PTR [rbp - 16], 1
  mov rbx, rax
  mov rax, QWORD PTR [rbp - 8]
  add QWORD PTR [rbp - 8], 1
  mov r10b, BYTE PTR [rbx]
  mov BYTE PTR [rax], r10b
  jmp L0
L1:
Lend_string_copy:
  pop r10
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 
string_concat:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  push r10
  mov QWORD PTR [rbp - 8], rdi
  mov QWORD PTR [rbp - 16], rsi
L2:
  mov rax, QWORD PTR [rbp - 8]
  mov bl, BYTE PTR [rax]
  test bl, bl
  je L3
  add QWORD PTR [rbp - 8], 1
  jmp L2
L3:
L4:
  mov rax, QWORD PTR [rbp - 16]
  mov bl, BYTE PTR [rax]
  test bl, bl
  je L5
  mov rax, QWORD PTR [rbp - 16]
  add QWORD PTR [rbp - 16], 1
  mov rbx, rax
  mov rax, QWORD PTR [rbp - 8]
  add QWORD PTR [rbp - 8], 1
  mov r10b, BYTE PTR [rbx]
  mov BYTE PTR [rax], r10b
  jmp L4
L5:
Lend_string_concat:
  pop r10
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 
string_length:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push rbx
  mov QWORD PTR [rbp - 16], rdi
  mov QWORD PTR [rbp - 8], 0
L6:
  mov rax, QWORD PTR [rbp - 16]
  add rax, QWORD PTR [rbp - 8]
  mov bl, BYTE PTR [rax]
  test bl, bl
  je L7
  mov rax, QWORD PTR [rbp - 8]
  add QWORD PTR [rbp - 8], 1
  jmp L6
L7:
  mov rax, QWORD PTR [rbp - 8]
  jmp Lend_string_length
Lend_string_length:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 
main:
  push rbp
  mov rbp, rsp
  sub rsp, 32
  push rbx
  mov DWORD PTR [rbp - 20], edi
  mov QWORD PTR [rbp - 28], rsi
  cmp DWORD PTR [rbp - 20], 3
  setne al
  movzx eax, al
  test eax, eax
  je L8
  mov rax, QWORD PTR [rbp - 28]
  lea rax, [rax + 0]
  mov rsi, QWORD PTR [rax]
  lea rdi, [rip + .LS2]
  xor al, al
  call printf
  mov eax, 1
  jmp Lend_main
L8:
  mov rax, QWORD PTR [rbp - 28]
  lea rax, [rax + 8]
  mov rdi, QWORD PTR [rax]
  call atoi
  mov ebx, eax
  mov DWORD PTR [rbp - 4], ebx
  mov rax, QWORD PTR [rbp - 28]
  lea rax, [rax + 16]
  mov rdi, QWORD PTR [rax]
  call atoi
  mov ebx, eax
  mov DWORD PTR [rbp - 8], ebx
  cmp DWORD PTR [rbp - 4], 0
  jne L9
  cmp DWORD PTR [rbp - 8], 0
  jne L9
  xor al, al
  jmp L10
L9:
  mov al, 1
L10:
  test al, al
  je L11
  lea rdi, [rip + .LS3]
  call puts
  jmp L12
L11:
  lea rdi, [rip + .LS4]
  call puts
L12:
  lea rax, [rbp - 4]
  mov QWORD PTR [rbp - 16], rax
  mov rax, QWORD PTR [rbp - 16]
  mov esi, DWORD PTR [rax]
  lea rdi, [rip + .LS5]
  xor al, al
  call printf
  xor eax, eax
  jmp Lend_main
Lend_main:
  pop rbx
  mov rsp, rbp
  pop rbp
  ret 

.section .note.GNU-stack,"",@progbits
