CC=gcc
CFLAGS=-I./include
build_path = ./build
src_path = ./src
SRC = ttf_utils.c ttf_tables.c ttf_reader.c ttf_raster.c
OBJ = $(patsubst %.c, $(build_path)/%.o, $(SRC))

default: clean lib 
	./a.out ubuntu.ttf

lib: $(OBJ) 
	ar -rc $(build_path)/libttf.a $^

$(build_path)/%.o: $(src_path)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: example
example: clean lib 
	ls ./build
	gcc ./example/draw.c -I./include $(build_path)/libttf.a `pkg-config --libs --cflags cairo` \
			`pkg-config --cflags --libs gtk+-2.0` \
		-o a.out && ./a.out


clean: 
	rm -r build
	mkdir build
