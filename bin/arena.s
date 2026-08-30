.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	LR3:	.asciz "malloc() error\n"
	LR4:	.asciz "Maximum size reached for arena.\n"
	LR5:	.asciz "%d, %d\n"
	LR6:	.asciz "Enter magic value: "
	LR7:	.asciz "%d"

.section .text
.global main

make_arena:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  push	rbx
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	QWORD PTR [rax + 16], 16384
  mov	rax, QWORD PTR [rbp - 8]
  mov	QWORD PTR [rax + 8], 0
  mov	rax, QWORD PTR [rbp - 8]
  mov	rdi, QWORD PTR [rax + 16]
  call	malloc
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 8]
  mov	QWORD PTR [rax + 0], rbx
  mov	rax, QWORD PTR [rbp - 8]
  mov	rax, QWORD PTR [rax + 0]
  test	rax, rax
  sete	al
  movzx	rax, al
  test	rax, rax
  je	L0
  lea	rdi, [rip + LR3]
  xor	al, al
  call	printf
  mov	edi, 1
  call	exit
L0:
Lend_make_arena:
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
arena_push:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 32
  push	rbx
  push	r12
  push	r13
  mov	QWORD PTR [rbp - 16], rdi
  mov	QWORD PTR [rbp - 24], rsi
  mov	QWORD PTR [rbp - 8], 0
  mov	rax, QWORD PTR [rbp - 16]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbx + 8]
  add	rax, QWORD PTR [rbp - 24]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 16]
  cmp	rbx, QWORD PTR [rax + 16]
  setg	al
  movzx	eax, al
  test	eax, eax
  je	L1
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
  mov	rax, QWORD PTR [rbp - 8]
  jmp	Lend_arena_push
L1:
  mov	rax, QWORD PTR [rbp - 16]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 16]
  mov	rax, QWORD PTR [rbp - 16]
  mov	r12, rax
  mov	rax, QWORD PTR [rbp - 16]
  mov	rax, QWORD PTR [rax + 8]
  imul	rax, 8
  mov	r13, rax
  mov	rax, QWORD PTR [r12 + 0]
  add	rax, r13
  mov	rbx, rax
  mov	QWORD PTR [rbp - 8], rbx
  mov	rax, QWORD PTR [rbp - 16]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbx + 8]
  add	rax, QWORD PTR [rbp - 24]
  mov	rbx, rax
  mov	rax, QWORD PTR [rbp - 16]
  mov	QWORD PTR [rax + 8], rbx
  mov	rax, QWORD PTR [rbp - 8]
  jmp	Lend_arena_push
Lend_arena_push:
  pop	r13
  pop	r12
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	
print_item:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	edx, DWORD PTR [rax + 4]
  mov	rax, QWORD PTR [rbp - 8]
  mov	esi, DWORD PTR [rax + 0]
  lea	rdi, [rip + LR5]
  xor	al, al
  call	printf
Lend_print_item:
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 48
  lea	rax, [rbp - 24]
  mov	rdi, rax
  call	make_arena
  mov	rsi, 8
  lea	rax, [rbp - 24]
  mov	rdi, rax
  call	arena_push
  mov	QWORD PTR [rbp - 32], rax
  lea	rdi, [rip + LR6]
  xor	al, al
  call	printf
  lea	rax, [rbp - 36]
  mov	rsi, rax
  lea	rdi, [rip + LR7]
  xor	al, al
  call	scanf
  cmp	DWORD PTR [rbp - 36], 0
  setg	al
  movzx	eax, al
  cmp	QWORD PTR [rbp - 32], 0
  je	L3
  cmp	eax, 0
  je	L3
  mov	al, 1
  jmp	L2
L3:
  xor	al, al
L2:
  test	al, al
  je	L4
  mov	rax, QWORD PTR [rbp - 32]
  mov	DWORD PTR [rax + 0], 5
  mov	rax, QWORD PTR [rbp - 32]
  mov	DWORD PTR [rax + 4], 10
L4:
  mov	rdi, QWORD PTR [rbp - 32]
  call	print_item
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
