global ConditionalNOP

section .text

;
; NOTE(casey): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (the count),
; and if applicable, RDX to be the second parameter (the data pointer).
; To use these on a platform with a different ABI, you would have to
; change those registers to match the ABI.
;

ConditionalNOP:
    xor rax, rax
.loop:
    mov r10, [rdx + rax]
	inc rax
	test r10, 1
    jnz .skip
	nop
.skip:
    cmp rax, rcx
    jb .loop
    ret