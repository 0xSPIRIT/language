.intel_syntax noprefix

.section .rodata
.LS1: .asciz "r"
.LS2: .asciz "Couldn't open the file."
.LS3: .asciz "%p\n"
.LS4: .asciz "%d\n"
.LS5: .asciz "Usage: %s filename\n"

.section .text
.global main

nop:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 0
Lend_nop:
  mov	rsp, rbp
  pop	rbp
  ret	
read_entire_file:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 48
  mov	QWORD PTR [rbp - 40], rdi
  lea	rsi, [rip + .LS1]
  mov	rdi, QWORD PTR [rbp - 40]
  call	fopen
  mov	QWORD PTR [rbp - 8], rax
  mov	rax, QWORD PTR [rbp - 8]
  test	rax, rax
  sete	al
  movzx	rax, al
  test	rax, rax
  je	L0
  lea	rdi, [rip + .LS2]
  xor	al, al
  call	printf
  xor	eax, eax
  jmp	Lend_read_entire_file
L0:
  mov	edx, 2
  xor	esi, esi
  mov	rdi, QWORD PTR [rbp - 8]
  call	fseek
  mov	rdi, QWORD PTR [rbp - 8]
  call	ftell
  mov	DWORD PTR [rbp - 16], eax
  mov	rdi, QWORD PTR [rbp - 8]
  call	rewind
  mov	rax, QWORD PTR [rbp - 16]
  add	rax, 1
  mov	rdi, rax
  call	malloc
  mov	QWORD PTR [rbp - 24], rax
  mov	rcx, QWORD PTR [rbp - 8]
  mov	rdx, QWORD PTR [rbp - 16]
  mov	rsi, 1
  mov	rdi, QWORD PTR [rbp - 24]
  call	fread
  mov	QWORD PTR [rbp - 32], rax
  mov	rax, QWORD PTR [rbp - 24]
  add	rax, QWORD PTR [rbp - 32]
  mov	BYTE PTR [rax], 0
  mov	rdi, QWORD PTR [rbp - 8]
  call	fclose
  mov	rdi, QWORD PTR [rbp - 24]
  call	puts
  mov	rdi, QWORD PTR [rbp - 24]
  call	free
  mov	eax, 1
  jmp	Lend_read_entire_file
Lend_read_entire_file:
  mov	rsp, rbp
  pop	rbp
  ret	
linked_list:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  mov	rdi, 12
  call	malloc
  mov	QWORD PTR [rbp - 8], rax
  mov	rax, QWORD PTR [rbp - 8]
  mov	DWORD PTR [rax + 0], 5
  mov	rdi, 12
  call	malloc
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 8]
  mov	QWORD PTR [rax + 4], rbx
  mov	rax, QWORD PTR [rbp - 8]
  mov	rax, QWORD PTR [rax + 4]
  mov	DWORD PTR [rax + 0], 10
  mov	rax, QWORD PTR [rbp - 8]
  mov	rax, QWORD PTR [rax + 4]
  mov	QWORD PTR [rax + 4], 0
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + .LS3]
  xor	al, al
  call	printf
  mov	rax, QWORD PTR [rbp - 8]
  mov	esi, DWORD PTR [rax + 0]
  lea	rdi, [rip + .LS4]
  xor	al, al
  call	printf
  mov	rax, QWORD PTR [rbp - 8]
  mov	rsi, QWORD PTR [rax + 4]
  lea	rdi, [rip + .LS3]
  xor	al, al
  call	printf
  mov	rax, QWORD PTR [rbp - 8]
  mov	rax, QWORD PTR [rax + 4]
  mov	esi, DWORD PTR [rax + 0]
  lea	rdi, [rip + .LS4]
  xor	al, al
  call	printf
Lend_linked_list:
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	DWORD PTR [rbp - 4], edi
  mov	QWORD PTR [rbp - 12], rsi
  cmp	DWORD PTR [rbp - 4], 2
  setne	al
  movzx	eax, al
  test	eax, eax
  je	L1
  mov	rax, QWORD PTR [rbp - 12]
  mov	rsi, QWORD PTR [rax]
  lea	rdi, [rip + .LS5]
  xor	al, al
  call	printf
  xor	eax, eax
  jmp	Lend_main
L1:
  mov	rax, QWORD PTR [rbp - 12]
  lea	rax, [rax + 8]
  mov	rdi, QWORD PTR [rax]
  call	read_entire_file
  mov	eax, 67
  jmp	Lend_main
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
