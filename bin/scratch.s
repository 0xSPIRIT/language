.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "%p\n"

.section .text
.global main

print_ptr:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + LR3]
  xor	al, al
  call	printf
Lend_print_ptr:
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  mov	QWORD PTR [rbp - 8], 0
  mov	rdi, QWORD PTR [rbp - 8]
  call	print_ptr
  mov	rdi, 1000
  call	malloc
  mov	rbx, rax
  mov	QWORD PTR [rbp - 8], rbx
  mov	rdi, QWORD PTR [rbp - 8]
  call	print_ptr
  mov	rdi, QWORD PTR [rbp - 8]
  call	free
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
