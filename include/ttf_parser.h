#ifndef TTF_PARSER
#define TTF_PARSER

#include "ttf_libc.h"
#include "ttf_tables.h"
#include "ttf_utils.h"
#include "ttf_tables.h"

typedef struct TTF_FONT {
    uint16_t glyfs_n;
    TTF_GLYF** glyfs;
    TTF_TABLE_HEAD* head;
    TTF_TABLE_CMAP* cmap;
    void (*free)(struct TTF_FONT*);
} TTF_FONT;

void extract_glyfs(uint8_t* ttf_buffer, TTF_FONT** font_out);

TTF_GLYF* glyf_from_char(TTF_FONT* font, uint8_t ch);

void extract_font_from_file(char* fname, TTF_FONT** font);

#endif
