.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "n = "
	LR4:	.asciz "%d"
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
  mov	DWORD PTR [rbp - 4], edi
  cmp	DWORD PTR [rbp - 4], 1
  setle	al
  movzx	eax, al
  test	eax, eax
  je	L0
  mov	eax, DWORD PTR [rbp - 4]
  jmp	Lend_fibonacci
L0:
  mov	eax, DWORD PTR [rbp - 4]
  sub	eax, 1
  mov	edi, eax
  call	fibonacci
  mov	ebx, eax
  mov	eax, DWORD PTR [rbp - 4]
  sub	eax, 2
  mov	edi, eax
  call	fibonacci
  mov	r12d, eax
  mov	eax, ebx
  add	eax, r12d
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
  mov	DWORD PTR [rbp - 4], edi
  cmp	DWORD PTR [rbp - 4], 1
  setle	al
  movzx	eax, al
  test	eax, eax
  je	L1
  mov	eax, DWORD PTR [rbp - 4]
  jmp	Lend_factorial
L1:
  mov	eax, DWORD PTR [rbp - 4]
  sub	eax, 1
  mov	edi, eax
  call	factorial
  mov	ebx, eax
  mov	eax, DWORD PTR [rbp - 4]
  imul	eax, ebx
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
  lea	rax, [rbp - 4]
  mov	rsi, rax
  lea	rdi, [rip + LR4]
  xor	al, al
  call	scanf
  mov	edi, DWORD PTR [rbp - 4]
  call	fibonacci
  mov	edx, eax
  mov	esi, DWORD PTR [rbp - 4]
  lea	rdi, [rip + LR5]
  xor	al, al
  call	printf
  mov	edi, DWORD PTR [rbp - 4]
  call	factorial
  mov	edx, eax
  mov	esi, DWORD PTR [rbp - 4]
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
