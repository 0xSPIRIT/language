.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "The pointer is %p and the value is (%d %d %d)\n"
	LR4:	.asciz "You're a wizard, Harry!\n"
	LR5:	.asciz "You're a fucking muggle, Harry!\n"

.section .bss
	test:	.space 12

.section .text
.global main

print_vec:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	r8d, DWORD PTR [rax + 8]
  mov	rax, QWORD PTR [rbp - 8]
  mov	ecx, DWORD PTR [rax + 4]
  mov	rax, QWORD PTR [rbp - 8]
  mov	edx, DWORD PTR [rax + 0]
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + LR3]
  xor	al, al
  call	printf
Lend_print_vec:
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 0
  push	rbx
  push	r12
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 0], 1
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 4], 2
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 8], 3
  lea	rax, [rip + test]
  mov	rbx, rax
  lea	rax, [rip + test]
  mov	r12d, DWORD PTR [rax + 8]
  mov	eax, DWORD PTR [rbx + 0]
  add	eax, r12d
  mov	ebx, eax
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 0], ebx
  lea	rax, [rip + test]
  mov	rbx, rax
  lea	rax, [rip + test]
  mov	r12d, DWORD PTR [rax + 4]
  mov	eax, DWORD PTR [rbx + 0]
  add	eax, r12d
  mov	ebx, eax
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 0], ebx
  lea	rax, [rip + test]
  cmp	DWORD PTR [rax + 0], 5
  setl	al
  movzx	eax, al
  test	eax, eax
  je	L0
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
  jmp	L1
L0:
  lea	rdi, [rip + LR5]
  xor	al, al
  call	printf
L1:
  lea	rax, [rip + test]
  mov	rdi, rax
  call	print_vec
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  pop	r12
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
