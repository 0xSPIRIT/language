.intel_syntax noprefix

.section .rodata
	SEEK_SET:	.long	0
	SEEK_CUR:	.long	1
	SEEK_END:	.long	2
	SIZE:	.long	10
	LR4:	.asciz "[Entry @ %p] %c (%d, %d, %d)\n"
	LR5:	.asciz "sizeof(Entry) is %zu bytes\n"
	LR6:	.asciz "%d\n"

.section .text
.global main

print_entry:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	QWORD PTR [rbp - 8], rdi
  mov	rax, QWORD PTR [rbp - 8]
  mov	r9d, DWORD PTR [rax + 9]
  mov	rax, QWORD PTR [rbp - 8]
  mov	r8d, DWORD PTR [rax + 5]
  mov	rax, QWORD PTR [rbp - 8]
  mov	ecx, DWORD PTR [rax + 1]
  mov	rax, QWORD PTR [rbp - 8]
  mov	dl, BYTE PTR [rax + 0]
  mov	rsi, QWORD PTR [rbp - 8]
  lea	rdi, [rip + LR4]
  xor	al, al
  call	printf
Lend_print_entry:
  mov	rsp, rbp
  pop	rbp
  ret	
main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 160
  push	rbx
  mov	rsi, 13
  lea	rdi, [rip + LR5]
  xor	al, al
  call	printf
  mov	eax, 130
  cdq	
  mov	ebx, 13
  idiv	ebx
  mov	DWORD PTR [rbp - 134], eax
  lea	rax, [rbp - 134]
  mov	QWORD PTR [rbp - 142], rax
  mov	rax, QWORD PTR [rbp - 142]
  mov	QWORD PTR [rax], 20
  mov	esi, DWORD PTR [rbp - 134]
  lea	rdi, [rip + LR6]
  xor	al, al
  call	printf
  mov	DWORD PTR [rbp - 146], 0
L0:
  mov	ebx, DWORD PTR [rbp - 146]
  cmp	ebx, DWORD PTR [rbp - 134]
  setl	al
  movzx	eax, al
  test	eax, eax
  je	L1
  mov	eax, DWORD PTR [rbp - 146]
  imul	eax, 13
  movsx	rbx, eax
  lea	rax, [rbp - 130]
  add	rax, rbx
  mov	QWORD PTR [rbp - 154], rax
  mov	rax, QWORD PTR [rbp - 154]
  mov	BYTE PTR [rax + 0], 84
  mov	rax, QWORD PTR [rbp - 154]
  mov	ebx, DWORD PTR [rbp - 146]
  mov	DWORD PTR [rax + 1], ebx
  mov	rax, QWORD PTR [rbp - 154]
  mov	DWORD PTR [rax + 5], 5
  mov	rax, QWORD PTR [rbp - 154]
  mov	DWORD PTR [rax + 9], 6
L2:
  mov	eax, DWORD PTR [rbp - 146]
  add	DWORD PTR [rbp - 146], 1
  jmp	L0
L1:
  mov	DWORD PTR [rbp - 150], 0
L3:
  mov	ebx, DWORD PTR [rbp - 150]
  cmp	ebx, DWORD PTR [rbp - 134]
  setl	al
  movzx	eax, al
  test	eax, eax
  je	L4
  mov	eax, DWORD PTR [rbp - 150]
  imul	eax, 13
  movsx	rbx, eax
  lea	rax, [rbp - 130]
  add	rax, rbx
  mov	rdi, rax
  call	print_entry
L5:
  mov	eax, DWORD PTR [rbp - 150]
  add	DWORD PTR [rbp - 150], 1
  jmp	L3
L4:
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  pop	rbx
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
