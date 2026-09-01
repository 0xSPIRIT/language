.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "%d\n"
	LR4:	.asciz "Comparison is %d\n"

.section .text
.global main

main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  call	first23
  mov	DWORD PTR [rbp - 4], eax
  call	second26
  mov	DWORD PTR [rbp - 8], eax
  mov	eax, DWORD PTR [rbp - 4]
  add	eax, DWORD PTR [rbp - 8]
  mov	esi, eax
  lea	rdi, [rip + LR3]
  xor	al, al
  call	printf
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	
first23:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 0
  call	example24
  mov	eax, 10
  jmp	Lend_first23
Lend_first23:
  mov	rsp, rbp
  pop	rbp
  ret	
second26:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 0
  call	example27
  mov	eax, 10
  jmp	Lend_second26
Lend_second26:
  mov	rsp, rbp
  pop	rbp
  ret	
example24:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	DWORD PTR [rbp - 8], 10
  mov	DWORD PTR [rbp - 4], 20
  lea	rax, [rbp - 8]
  mov	rdi, rax
  call	compare25
  mov	esi, eax
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
Lend_example24:
  mov	rsp, rbp
  pop	rbp
  ret	
example27:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	DWORD PTR [rbp - 8], 10
  mov	DWORD PTR [rbp - 4], 20
  lea	rax, [rbp - 8]
  mov	rdi, rax
  call	compare28
  mov	esi, eax
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
Lend_example27:
  mov	rsp, rbp
  pop	rbp
  ret	
compare25:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  push	r12
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 8]
  mov	r12d, DWORD PTR [rbx]
  cmp	r12d, DWORD PTR [rax + 4]
  setg	al
  movzx	eax, al
  jmp	Lend_compare25
Lend_compare25:
  pop	r12
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
compare28:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  push	r12
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	ebx, DWORD PTR [rax]
  mov	eax, 67
  add	eax, ebx
  mov	ebx, eax
  mov	rax, QWORD PTR [rbp - 8]
  mov	r12d, DWORD PTR [rax + 4]
  mov	eax, ebx
  add	eax, r12d
  jmp	Lend_compare28
Lend_compare28:
  pop	r12
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
