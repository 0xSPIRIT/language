.global _start

.section .data
    msg:
      .ascii "This is an ASCII string!\n"
    msg_end:

.equ msg_len, msg_end-msg

.section .text
_start:
  movl $4, %eax # 4 = write
  movl $1, %ebx # 1 = stdout fd
  movl $msg, %ecx # data buffer
  movl $msg_len, %edx # length

  int $0x80 # Trigger syscall

  movl $1, %eax
  movl $67, %ebx
  int $0x80

.section .note.GNU-stack,"",@progbits
