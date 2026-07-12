https://github.com/namantam1/x86-assembly/tree/main#conditional-statement-ifelse

# Static Data Regions

.data # Use this to declare global variables (can be written & read)

var1:
    .byte 64  /* Location is var1 */
    .byte 10  /* Location is var1+1 */
    .byte 3   /* Location is var1+2 */

x:
    .short 42 /* Declare 2-byte value as 42 */

arr:
    .long 1, 2, 3  /* Declare array of longs, arr + 8 will refer to 3 */

barr:
    .zero 10 /* Declare 10 bytes initialized to 0 */

# Addressing Modes

mov (%ebx), %eax  /* Load 4 bytes from EBX into EAX */
mov %ebx, var(,1) /* Move EBX into the 4 bytes at memory address var,
                     with (,1) representing the scale factor */
mov -4(%esi), %eax /* Move 4 bytes at ESI-4 into EAX */
mov %cl, (%esi, %eax, 1) /* *(ESI+EAX) = CL */
mov (%esi,%ebx,4), %edx /* EDX = *(ESI + 4*EBX)

Invalid: mov (%ebx,%ecx,-1), %eax /* Can only add registers */

## Memory Addressing Syntax

You dereference memory using () (similar to * in C)
The size of the data being dereferenced is inferred by the instruction,
i.e., movb (1), movw (2), movl (4)

If the size is not explicit, mov will infer the size of the data item
being referenced by the size of the register operand.

Example: (%rax) gets the data at %rax
D(Rb, Ri, S) => Rb + Ri * S + D   /* D is signed, S = { 1, 2, 4, 8 } */

## Stack

The hardware stack grows downwards, towards lower addresses.

pushl %reg
popl %reg

## Registers

The order for passing syscall parameters are
  0. ebx
  1. ecx
  2. edx
  3. esi
  4. edi
  5. ebp

To do a syscall set %eax to the syscall code, and then set the parameters up in the registers
before calling int $0x80

ebp - Extended base pointer, Frame Pointer.
    - Used when entering a function, refers to the top of the stack.
    - %esp will continue being moved as data is pushed/popped

## Instructions

cmp src, dest   # Computes dest - src
jge label       # Jumps if dest >= src

## Function Calls

1) push function parameters in reverse order using pushl
2) call function using call function-name
3) define the function name as .type function-name, @function
4) start the function block with the function name (.function-name:)
5) Function Prologue:
    pushl %ebp
    movl  %esp, %ebp    # ebp (base frame pointer) = esp (top of stack)
6) Get function parameters by using %ebp with offsets
    movl 8(%ebp), %ebx  # Load arg0 into %ebx
    (Arguments must start at 8, since 0-3 is taken by the saved %ebp and 4-7 is taken by
     the 4-byte return address that will be jumped to after ret is called.)
7) Do the things in the function
8) Store return value in %eax
9) Function Epilogue:
     movl %ebp, %esp     # Deallocate locals, restore stack pointer
     popl %ebp
     ret
