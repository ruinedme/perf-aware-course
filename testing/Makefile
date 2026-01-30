multinop_loops:
	cl /TC /W4 /nologo /O2 /Febin\multinop_loops.exe multinop_loops_main.c

multinop_asm:
	nasm -f win64 -o bin/multinop_loops.obj multinop_loops.asm && lib -nologo -out:bin/multinop_loops.lib bin/multinop_loops.obj

conditional_nop_loops:
	cl /TC /W4 /nologo /O2 /Febin\conditional_nop_loops.exe conditional_nop_loops_main.c

conditional_nop_asm:
	nasm -f win64 -o bin/conditional_nop_loops.obj conditional_nop_loops.asm && lib -nologo -out:bin/conditional_nop_loops.lib bin/conditional_nop_loops.obj

read_unroll_asm:
	nasm -f win64 -o bin/read_unroll.obj read_unroll.asm && lib -nologo -out:bin/read_unroll.lib bin/read_unroll.obj

read_unroll:
	cl /TC /W4 /nologo /O2 /Febin\read_unroll.exe read_unroll.c

write_unroll_asm:
	nasm -f win64 -o bin/write_unroll.obj write_unroll.asm && lib -nologo -out:bin/write_unroll.lib bin/write_unroll.obj

write_unroll:
	cl /TC /W4 /nologo /O2 /Febin\write_unroll.exe write_unroll.c

byte_read_asm:
	nasm -f win64 -o bin/byte_read.obj byte_read.asm && lib -nologo -out:bin/byte_read.lib bin/byte_read.obj

byte_read:
	cl /TC /W4 /nologo /O2 /Febin\byte_read.exe byte_read.c

read_width_asm:
	nasm -f win64 -o bin/read_width.obj read_width.asm && lib -nologo -out:bin/read_width.lib bin/read_width.obj

read_width:
	cl /TC /W4 /nologo /O2 /Febin\read_width.exe read_width.c

bandwidth_asm:
	nasm -f win64 -o bin/bandwidth.obj bandwidth.asm && lib -nologo -out:bin/bandwidth.lib bin/bandwidth.obj

bandwidth:
	cl /TC /W4 /nologo /O2 /Zi /Febin\bandwidth.exe bandwidth.c

unaligned_bandwidth_asm:
	nasm -f win64 -o bin/unaligned_bandwidth.obj unaligned_bandwidth.asm && lib -nologo -out:bin/unaligned_bandwidth.lib bin/unaligned_bandwidth.obj

unaligned_bandwidth:
	cl /TC /W4 /nologo /O2 /Zi /Febin\unaligned_bandwidth.exe unaligned_bandwidth.c