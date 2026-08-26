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
  push	rbx
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	rax, QWORD PTR [rax]
  add	rax, 1
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 8]
  mov	QWORD PTR [rax], rbx
Lend_modify_value:
  pop	rbx
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
