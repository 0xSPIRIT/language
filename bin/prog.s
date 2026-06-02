.global main

.section .data
    msg: .string "This is an ASCII string!"

.section .text
main:
    # Set up stack frame (Standard IA32 Calling Convention)
    pushl %ebp
    movl %esp, %ebp

    # Push function arguments to stack from right to left
    pushl $msg
    call puts
    addl $4, %esp        # Clean up the 4-byte argument pointer from stack

    movl $67, %eax

    # Restore stack frame and exit
    movl %ebp, %esp
    popl %ebp
    ret

.section .note.GNU-stack,"",@progbits
