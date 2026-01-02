# Unoptimized build
build0:
	gcc -std=c99 -O0 -Wall -Wextra -Wpedantic -shared -o bin/json_parser0.dll -Wl,--out-implib,bin/libjson_parser2.a -Iincludes/ src/*
#  --help={common|optimizers|params|target|warnings|[^]{joined|separate|undocumented}}[,...].

# some optimations
build2:
	gcc -std=c99 -O2 -Wall -Wextra -Wpedantic -shared -o bin/json_parser2.dll -Wl,--out-implib,bin/libjson_parser2.a -Iincludes/ src/*

# aggressive optimization
build build3:
	gcc -std=c99 -O3 -Wall -Wextra -Wpedantic -g -shared -o bin/json_parser3.dll -Wl,--out-implib,bin/libjson_parser3.a -Iincludes/ src/* -static-libgcc