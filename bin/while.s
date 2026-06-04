.globl _start

.section .text
_start:
  movl $0, %ebx   # a = 0
  movl $10, %ecx  # b = 10

loop:
  addl %ecx, %ebx # a += b
  decl %ecx       # b--

end:
  cmpl $0, %ecx
  jne loop        # while (b != 0)

  movl $1, %eax
  int $0x80       # return a;
