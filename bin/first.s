.intel_syntax noprefix

.section .rodata
	LR0:	.asciz "%d\n"

.section .text
.global main

main:
  push	rbp
  mov	rbp, rsp
  sub	rsp, 16
  mov	DWORD PTR [rbp - 4], 5
  mov	DWORD PTR [rbp - 8], 6
  mov	eax, DWORD PTR [rbp - 4]
  add	eax, DWORD PTR [rbp - 8]
  mov	DWORD PTR [rbp - 12], eax
  mov	esi, DWORD PTR [rbp - 12]
  lea	rdi, [rip + LR0]
  xor	al, al
  call	printf
  xor	eax, eax
  jmp	Lend_main
Lend_main:
  mov	rsp, rbp
  pop	rbp
  ret	

.section .note.GNU-stack,"",@progbits
