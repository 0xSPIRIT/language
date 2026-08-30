.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "n = "
	LR4:	.asciz "%zu"
	LR5:	.asciz "fibonacci(%d) = %d\n"
	LR6:	.asciz "factorial(%d) = %d\n"

.section .text
.global main

fibonacci:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  push	r12
  mov	QWORD PTR [rbp - 8], rdi
  cmp	QWORD PTR [rbp - 8], 1
  setle	al
  movzx	eax, al
  test	eax, eax
  je	L0
  mov	rax, QWORD PTR [rbp - 8]
  jmp	Lend_fibonacci
L0:
  mov	rax, QWORD PTR [rbp - 8]
  sub	rax, 1
  mov	rdi, rax
  call	fibonacci
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 8]
  sub	rax, 2
  mov	rdi, rax
  call	fibonacci
  mov	r12, rax
  mov	rax, rbx
  add	rax, r12
  jmp	Lend_fibonacci
Lend_fibonacci:
  pop	r12
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
factorial:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  mov	QWORD PTR [rbp - 8], rdi
  cmp	QWORD PTR [rbp - 8], 1
  setle	al
  movzx	eax, al
  test	eax, eax
  je	L1
  mov	rax, QWORD PTR [rbp - 8]
  jmp	Lend_factorial
L1:
  mov	rax, QWORD PTR [rbp - 8]
  sub	rax, 1
  mov	rdi, rax
  call	factorial
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 8]
  imul	rax, rbx
  jmp	Lend_factorial
Lend_factorial:
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  lea	rdi, [rip + LR3]
  xor	al, al
  call	printf
  lea	rax, [rbp - 8]
  mov	rsi, rax
  lea	rdi, [rip + LR4]
  xor	al, al
  call	scanf
  mov	rdi, QWORD PTR [rbp - 8]
  call	fibonacci
  mov	rdx, rax
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + LR5]
  xor	al, al
  call	printf
  mov	rdi, QWORD PTR [rbp - 8]
  call	factorial
  mov	rdx, rax
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + LR6]
  xor	al, al
  call	printf
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
