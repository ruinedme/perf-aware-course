; Test reading 1gb of memory. Increase the window into the buffer across iterations
; example test reading the first 64k of a 1gb buffer repeatdly
; then test reading the frist 128k of 1gb buffer, and so on 
; last test should just read the whole 1gb buffer

; Windows 64-bit ABI. They expect RCX to be the first parameter (the count),
; and RDX to be the second (the data pointer)
; parameter (the data pointer)

; need to track number of bytes read
; need a mask to only read upto a certain part of the buffer

global Read_32x8


section .text

Read_32x8:
    xor r9, r9
    mov rax, rdx
    align 64
.loop:
    ; Read 256 bytes
    vmovdqu ymm0, [rax + 1]
    vmovdqu ymm0, [rax + 0x21]
    vmovdqu ymm0, [rax + 0x41]
    vmovdqu ymm0, [rax + 0x61]
    vmovdqu ymm0, [rax + 0x81]
    vmovdqu ymm0, [rax + 0xa1]
    vmovdqu ymm0, [rax + 0xc1]
    vmovdqu ymm0, [rax + 0xf1]

    ; Advance and mask the offset
    add r9, 0x100
    and r9, r8

    ; update read base pointer to new offset
    mov rax, rdx
    add rax, r9

    ; loop
    sub rcx, 0x100
    jnz .loop
    ret
