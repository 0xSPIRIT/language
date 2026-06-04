.globl _start

.section .text
_start:
  movl $1, %eax
  movl $10, %ebx

  cmpl $9, %ebx
  jne end_block
  movl $0, %ebx
end_block:
  int $0x80
