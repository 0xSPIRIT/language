.globl _start

.type sum @function
sum:
  pushl %ebp
  movl %esp, %ebp

  movl 8(%ebp), %ebx # ebx = 10
  movl 12(%ebp), %ecx # ecx = 20

  addl %ebx, %ecx  # ecx += ebx
  movl %ecx, %eax  # eax = ecx

  movl %ebp, %esp
  popl %ebp
  ret

.section .text
_start:
  pushl $20
  pushl $10
  call sum
  addl $8, %esp # cleanup stack

  movl %eax, %ebx
  movl $1, %eax
  int $0x80
