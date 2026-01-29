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
;  LISTING 143
;  ========================================================================

;
;  NOTE(casey): Regular Homework
;
                  ; My Attempt
                  ; dest | src 
    mov rax, 1    ; s1   | s0     ; s1 rax
    mov rbx, 2    ; s3   | s2     ; s3 rbx
    mov rcx, 3    ; s5   | s4     ; s5 rcx
    mov rdx, 4    ; s7   | s6     ; s7 rdx
    add rax, rbx  ; s8   | s3     ; s8 rax
	add rcx, rdx  ; s9   | s7     ; s9 rcx
	add rax, rcx  ; s8   | s9      
    mov rcx, rbx  ; s3   | s3     ; s3 rcx    
	inc rax       ; s10  | s8     ; s10 rax
    dec rcx       ; s11  | s3     ; s11 rcx
    sub rax, rbx  ; s12  | s3     ; s12 rax
	sub rcx, rdx  ; s13  | s7     ; s13 rcx
	sub rax, rcx  ; s14  | s13    ; s14 rax

;
;  NOTE(casey): CHALLENGE MODE WITH ULTIMATE DIFFICULTY SETTINGS
;               DO NOT ATTEMPT THIS! IT IS MUCH TOO HARD FOR
;               A HOMEWORK ASSIGNMENT!1!11!!
;

                 ; dest | src   ;
top:             ;      |       ;
    pop rcx      ;      |       ;
    sub rsp, rdx ;      |       ;
	mov rbx, rax ;      |       ;
	shl rbx, 0   ;      |       ;
	not rbx      ;      |       ;
	loopne top   ;      |       ;
