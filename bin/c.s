.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "(%d %d %d)\n"

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
  mov	ecx, DWORD PTR [rax + 8]
  mov	rax, QWORD PTR [rbp - 8]
  mov	edx, DWORD PTR [rax + 4]
  mov	rax, QWORD PTR [rbp - 8]
  mov	esi, DWORD PTR [rax + 0]
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
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 0], 1
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 4], 2
  lea	rax, [rip + test]
  mov	DWORD PTR [rax + 8], 3
  lea	rax, [rip + test]
  mov	rdi, rax
  call	print_vec
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
