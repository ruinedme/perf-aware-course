.PHONY: build

compile:
#   not entirely sure all of the linker options are required, but it works
#	still need to manually copy dll to working directory of the exe though
#	apparently the order of source files to linked files is important.
# 	gcc -std=c99 -O2 \
# 	-fno-fast-math -frounding-math -msse2 -mfpmath=sse \
# 	-Wall -Wextra -Wpedantic -o bin/main.exe \
# 	-I../sax_json -L../sax_json/bin -Wl,-rpath,'${ORIGIN}/../sax_json/bin' \
# 	main.c -lsax_json
#	cl /O2 /W4 /Febin\main.exe /DL /I..\sax_json ..\sax_json\bin\sax_json.obj main
	cl /c /TC /O2 /W4 /I..\sax_json /Fobin\main.obj main.c

link:
	cl /Febin\main.exe /DL /I..\sax_json ..\sax_json\bin\sax_json.obj bin\main.obj

build: compile link

# asm:
# 	gcc -std=c99 -O2 \
# 	-fno-fast-math -frounding-math -msse2 -mfpmath=sse -fexcess-precision=standard \
# 	-Wall -Wextra -Wpedantic -S -masm=intel -o bin/main.s \
# 	-I../sax_json -L../sax_json/bin -Wl,-rpath,'${ORIGIN}/../sax_json/bin' \
# 	main.c -lsax_json
# check for x87 instructions
# grep -E "fld|fstp|faddp|fsubp|fadd|fsub" ./bin/main.s
