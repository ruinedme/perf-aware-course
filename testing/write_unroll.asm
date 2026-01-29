;  ========================================================================
;
;  (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.
;
;  This software is provided 'as-is', without any express or implied
;  warranty. In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Please see https://computerenhance.com for more information
;
;  ========================================================================

;  ========================================================================
;  LISTING 144
;  ========================================================================

global Write_x1
global Write_x2
global Write_x3
global Write_x4

section .text

;
; NOTE(casey): These ASM routines are written for the Windows
; 64-bit ABI. They expect the count in rcx and the data pointer in rdx.
;

Write_x1:
	align 64
.loop:
    mov [rdx], rax
    sub rcx, 1
    jnle .loop
    ret

Write_x2:
	align 64
.loop:
    mov [rdx], rax
    mov [rdx], rax
    sub rcx, 2
    jnle .loop
    ret

Write_x3:
    align 64
.loop:
    mov [rdx], rax
    mov [rdx], rax
    mov [rdx], rax
    sub rcx, 3
    jnle .loop
    ret

Write_x4:
	align 64
.loop:
    mov [rdx], rax
    mov [rdx], rax
    mov [rdx], rax
    mov [rdx], rax
    sub rcx, 4
    jnle .loop
    ret