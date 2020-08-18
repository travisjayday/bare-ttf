#ifndef TTF_READER
#define TTF_READER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ttf_tables.h"
#include "ttf_utils.h"
#include "ttf_tables.h"

typedef struct {
    uint16_t glyfs_n;
    TTF_GLYF** glyfs;
    TTF_TABLE_HEAD* head;
} TTF_FONT;

void extract_glyfs(uint8_t* ttf_buffer, TTF_FONT** font_out);

void extract_font_from_file(char* fname, TTF_FONT** font);

#endif
