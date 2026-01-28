.PHONY: build

#compile:
#   not entirely sure all of the linker options are required, but it works
#	still need to manually copy dll to working directory of the exe though
#	apparently the order of source files to linked files is important.
# 	gcc -std=c99 -O2 \
# 	-fno-fast-math -frounding-math -msse2 -mfpmath=sse \
# 	-Wall -Wextra -Wpedantic -o bin/main.exe \
# 	-I../sax_json -L../sax_json/bin -Wl,-rpath,'${ORIGIN}/../sax_json/bin' \
# 	main.c -lsax_json


build:
# 	cl  /TC /W4 /nologo /O2 /Zi /Fdbin\main.pdb /Fobin\main.obj /Fmbin\main.map /Febin\main.exe main.c
	cl  /TC /W4 /nologo /O2 /Zi /arch:AVX2 main.c /FA /Fabin\main.asm /Fdbin\main.pdb /Fobin\main.obj /Fmbin\main.map /Febin\main.exe /link /DEBUG:FULL

release:
	cl  /TC /W4 /nologo /O2 /arch:AVX2 main.c /Fobin\main.obj /Febin\main_release.exe
# asm:
# 	gcc -std=c99 -O2 \
# 	-fno-fast-math -frounding-math -msse2 -mfpmath=sse -fexcess-precision=standard \
# 	-Wall -Wextra -Wpedantic -S -masm=intel -o bin/main.s \
# 	-I../sax_json -L../sax_json/bin -Wl,-rpath,'${ORIGIN}/../sax_json/bin' \
# 	main.c -lsax_json
# check for x87 instructions
# grep -E "fld|fstp|faddp|fsubp|fadd|fsub" ./bin/main.s

rep:
	cl /TC /W4 /nologo /O2 /Febin\rep_test.exe multinop_loops_main.c
