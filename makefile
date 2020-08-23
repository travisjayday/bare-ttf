CC=gcc
CFLAGS=-g -I./include
build_path = ./build
src_path = ./src
example_path = ./example
SRC = ttf_utils.c ttf_tables.c ttf_parser.c ttf_raster.c ttf_libc.c
OBJ = $(patsubst %.c, $(build_path)/%.o, $(SRC))

.PHONY: example
example: clean lib 
	$(CC) $(CFLAGS) $(example_path)/draw.c $(build_path)/libttf.a `pkg-config --libs --cflags cairo` \
			`pkg-config --cflags --libs gtk+-2.0` \
		-o $(build_path)/a.out && $(build_path)/./a.out $(example_path)/nobuntu.ttf

lib: $(OBJ) 
	ar -rc $(build_path)/libttf.a $^

$(build_path)/%.o: $(src_path)/%.c
	$(CC) $(CFLAGS) -c -o $@ $< $(CFLAGS)

clean: 
	rm -rf build
	mkdir build
