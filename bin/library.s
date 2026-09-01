.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "Lockwood and Co."
	LR4:	.asciz "Legendborne"
	LR5:	.asciz "Scythe"
	LR6:	.asciz "The Lightning Thief"
	LR7:	.asciz "I am Number Four"
	LR8:	.asciz "1984"
	LR9:	.asciz "Animal Farm"
	LR10:	.asciz "Bloodmarked"
	LR11:	.asciz "%d %d %d\n"
	LR12:	.asciz "%-25s (%d pages)\n"

.section .text
.global main

other_function:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	DWORD PTR [rbp - 12], 10
  mov	DWORD PTR [rbp - 8], 12
  mov	DWORD PTR [rbp - 4], 13
  lea	rax, [rbp - 12]
  mov	rdi, rax
  call	print_book23
Lend_other_function:
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 256
  push	rbx
  mov	DWORD PTR [rbp - 4], 0
  mov	edx, 200
  lea	rsi, [rip + LR3]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 215
  lea	rsi, [rip + LR4]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 321
  lea	rsi, [rip + LR5]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 324
  lea	rsi, [rip + LR6]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 345
  lea	rsi, [rip + LR7]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 500
  lea	rsi, [rip + LR8]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 200
  lea	rsi, [rip + LR9]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	edx, 212
  lea	rsi, [rip + LR10]
  lea	rax, [rbp - 244]
  mov	rdi, rax
  call	add_book25
  mov	DWORD PTR [rbp - 248], 0
L0:
  mov	ebx, DWORD PTR [rbp - 248]
  cmp	ebx, DWORD PTR [rbp - 4]
  setl	al
  movzx	eax, al
  test	eax, eax
  je	L1
  mov	eax, DWORD PTR [rbp - 248]
  imul	eax, 12
  movsx	rbx, eax
  lea	rax, [rbp - 244]
  add	rax, rbx
  mov	rdi, rax
  call	print_book28
L2:
  mov	eax, DWORD PTR [rbp - 248]
  add	DWORD PTR [rbp - 248], 1
  jmp	L0
L1:
  call	other_function
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
print_book23:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	ecx, DWORD PTR [rax + 8]
  mov	rax, QWORD PTR [rbp - 8]
  mov	edx, DWORD PTR [rax + 4]
  mov	rax, QWORD PTR [rbp - 8]
  mov	esi, DWORD PTR [rax]
  lea	rdi, [rip + LR11]
  xor	al, al
  call	printf
Lend_print_book23:
  mov	rsp, rbp
  pop	rbp
  ret	
add_book25:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 32
  push	rbx
  push	r12
  mov	QWORD PTR [rbp - 20], rdi
  mov	QWORD PTR [rbp - 28], rsi
  mov	DWORD PTR [rbp - 32], edx
  mov	eax, 240
  cdq	
  mov	ebx, 12
  idiv	ebx
  mov	DWORD PTR [rbp - 4], eax
  mov	rax, QWORD PTR [rbp - 20]
  mov	rbx, rax
  mov	r12d, DWORD PTR [rbx + 240]
  cmp	r12d, DWORD PTR [rbp - 4]
  setge	al
  movzx	eax, al
  test	eax, eax
  je	L3
  jmp	Lend_add_book25
L3:
  mov	rax, QWORD PTR [rbp - 20]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 20]
  mov	r11, rax
  mov	eax, DWORD PTR [r11 + 240]
  add	DWORD PTR [r11 + 240], 1
  imul	eax, 12
  movsx	r12, eax
  lea	rax, [rbx]
  add	rax, r12
  mov	QWORD PTR [rbp - 12], rax
  mov	rax, QWORD PTR [rbp - 12]
  mov	rbx, QWORD PTR [rbp - 28]
  mov	QWORD PTR [rax + 4], rbx
  mov	rax, QWORD PTR [rbp - 12]
  mov	ebx, DWORD PTR [rbp - 32]
  mov	DWORD PTR [rax], ebx
Lend_add_book25:
  pop	r12
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
print_book28:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	edx, DWORD PTR [rax]
  mov	rax, QWORD PTR [rbp - 8]
  mov	rsi, QWORD PTR [rax + 4]
  lea	rdi, [rip + LR12]
  xor	al, al
  call	printf
Lend_print_book28:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
