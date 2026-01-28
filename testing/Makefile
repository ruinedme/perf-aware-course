multinop_loops:
	cl /TC /W4 /nologo /O2 /Febin\multinop_loops.exe multinop_loops_main.c

multinop_asm:
	nasm -f win64 -o bin/multinop_loops.obj multinop_loops.asm && lib -nologo -out:bin/multinop_loops.lib bin/multinop_loops.obj

conditional_nop_loops:
	cl /TC /W4 /nologo /O2 /Febin\conditional_nop_loops.exe conditional_nop_loops_main.c

conditional_nop_asm:
	nasm -f win64 -o bin/conditional_nop_loops.obj conditional_nop_loops.asm && lib -nologo -out:bin/conditional_nop_loops.lib bin/conditional_nop_loops.obj