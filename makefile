CC=clang
inc = -I./include -I/usr/include/efi/x86_64 -I/usr/include/efi
cxx = -fno-stack-protector -fpic -Werror -Wno-error=incompatible-library-redeclaration -fshort-wchar -mno-red-zone -DHAVE_USE_MS_ABI -c 

build_path = ./build
src_path = ./src
example_path = ./example
SRC = ttf_utils.c ttf_tables.c ttf_parser.c ttf_raster.c ttf_libc.c
OBJ = $(patsubst %.c, $(build_path)/%.o, $(SRC))

.PHONY: example lib clean
example: clean _lib 
	gcc -g -O0 $(inc) $(example_path)/draw.c $(build_path)/libttf.a `pkg-config --libs --cflags cairo` \
			`pkg-config --cflags --libs gtk+-2.0` \
		-o $(build_path)/a.out && $(build_path)/./a.out $(example_path)/thinbuntu.ttf

libttf: clean _lib
	

_lib: $(OBJ) 
	ar -rc $(build_path)/libttf.a $^

$(build_path)/%.o: $(src_path)/%.c
	$(CC) $(inc) $(cxx) -o $@ $< $(CFLAGS)

clean: 
	rm -rf build
	mkdir build
