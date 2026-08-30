.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "ptr = %p\n"
	LR4:	.asciz "*ptr = %d\n"

.section .text
.global main

modify_value:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	r11, rax
  mov	rax, QWORD PTR [r11]
  add	QWORD PTR [r11], 1
Lend_modify_value:
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 32
  mov	DWORD PTR [rbp - 12], edi
  mov	QWORD PTR [rbp - 20], rsi
  xor	rdi, rdi
  call	malloc
  mov	QWORD PTR [rbp - 8], rax
  mov	rax, QWORD PTR [rbp - 8]
  mov	QWORD PTR [rax], 67
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + LR3]
  xor	al, al
  call	printf
  mov	rax, QWORD PTR [rbp - 8]
  mov	rsi, QWORD PTR [rax]
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
  mov	rdi, QWORD PTR [rbp - 8]
  call	modify_value
  mov	rax, QWORD PTR [rbp - 8]
  mov	rsi, QWORD PTR [rax]
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
  mov	rdi, QWORD PTR [rbp - 8]
  call	free
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
