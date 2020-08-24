#include "ttf_parser.h"

#define cmptag(t1, t2) \
    t1[0] == t2[0] && t1[1] == t2[1] && t1[2] == t2[2] && t1[3] == t2[3]

void free_font(TTF_FONT* font) {
    for (uint16_t i = 0; i < font->glyfs_n; i++) {
        if (font->glyfs[i] != NULL) {
            if (font->glyfs[i]->data != NULL) {
                // glyf is simple
                if (font->glyfs[i]->cont_n > 0) {
                    TTF_GLYF_SIMP_D* gdata = font->glyfs[i]->data;
                    gdata->free(font->glyfs[i]->data);
                }
                else {} // glyf is composite (aka NULL)
            }
            ttf_free(font->glyfs[i]);
        }
    }
    ttf_free(font->glyfs);
    ttf_free(font->head);
    if (font->cmap->format == 4) {
        TTF_CMAP_FMT4* fmt = (TTF_CMAP_FMT4*) font->cmap->encoding;
        ttf_free(fmt->start_c);
        ttf_free(fmt->end_c);
        ttf_free(fmt->id_delta);
        ttf_free(fmt->id_rng_offset);
        ttf_free(fmt->id_arr);
        ttf_free(fmt);
    }
    font->cmap->free(font->cmap);
    ttf_free(font);
}

   
void extract_font_from_bytes(uint8_t* ttf_buffer, TTF_FONT** font_out) {
    uint8_t* buf = ttf_buffer; 
    uint32_t cur = 4;

    uint16_t tables_n = read_uint16(buf, &cur);
    uint16_t search_range = read_uint16(buf, &cur);
    uint16_t entry_selector = read_uint16(buf, &cur);
    uint16_t range_shift = read_uint16(buf, &cur);

    ttf_log("Found 0x%x tables.\n", tables_n); 
    ttf_log("Found 0x%x search range.\n", search_range); 
    ttf_log("Found 0x%x entry selector.\n", entry_selector); 
    ttf_log("Found 0x%x range shift.\n", range_shift); 

    TTF_TABLE** tables = ttf_malloc(sizeof(TTF_TABLE*) * tables_n);
    
    TTF_TABLE_HEAD* head_t;
    TTF_TABLE_MAXP* maxp_t;
    TTF_TABLE_LOCA* loca_t;
    TTF_TABLE_CMAP* cmap_t;
    TTF_TABLE_HHEA* hhea_t;
    TTF_TABLE_HMTX* hmtx_t;

    uint8_t table_idx_maxp = -1;
    uint8_t table_idx_head = -1;
    uint8_t table_idx_glyf = -1;
    uint8_t table_idx_loca = -1;
    uint8_t table_idx_cmap = -1;
    uint8_t table_idx_hhea = -1;
    uint8_t table_idx_hmtx = -1;

    // search for tables in the main header
    for (uint8_t i = 0; i < tables_n; i++) {
        tables[i] = ttf_malloc(sizeof(TTF_TABLE));
        ttf_memcpy(tables[i]->tag, buf + cur, 4);
        cur += 4;
        tables[i]->tag[4] = '\0';
        tables[i]->checksum = read_uint32(buf, &cur);
        tables[i]->offset = read_uint32(buf, &cur);
        tables[i]->length = read_uint32(buf, &cur);
        ttf_log("Found table %s \n", tables[i]->tag); 
        ttf_log("\twith checksum 0x%x \n", tables[i]->checksum); 
        ttf_log("\twith offset 0x%x \n", tables[i]->offset); 
        ttf_log("\tand length 0x%x \n", tables[i]->length); 
        uint32_t check = calc_checksum(buf + tables[i]->offset, tables[i]->length);
        ttf_log("\tcomputed check: 0x%x\n", check);
        if (check == tables[i]->checksum)
            ttf_log("\tand correct checksum!\n", check); 
        else {
            if (cmptag(tables[i]->tag, "head")) {
                ttf_log("Making exception for incorrect `head` table checksum\n");
            }
            else {
                ttf_error("This table has an incorrect checksum. Corrupted .TTF?\n"); 
                ttf_error("Exiting... Expect unexpected behavior.\n");
                return;
            }
        }

        if      (cmptag(tables[i]->tag, "head")) table_idx_head = i;
        else if (cmptag(tables[i]->tag, "maxp")) table_idx_maxp = i;
        else if (cmptag(tables[i]->tag, "loca")) table_idx_loca = i;
        else if (cmptag(tables[i]->tag, "glyf")) table_idx_glyf = i;
        else if (cmptag(tables[i]->tag, "cmap")) table_idx_cmap = i;
        else if (cmptag(tables[i]->tag, "hhea")) table_idx_hhea = i;
        else if (cmptag(tables[i]->tag, "hmtx")) table_idx_hmtx = i;
    }
    
    // read tables into structs
    head_t = read_table_head(buf + tables[table_idx_head]->offset);
    maxp_t = read_table_maxp(buf + tables[table_idx_maxp]->offset);
    if (maxp_t == NULL) return;
    loca_t = read_table_loca(buf + tables[table_idx_loca]->offset, 
                maxp_t->glyfs_n, head_t->idx_to_loc_f);
    cmap_t = read_table_cmap(buf + tables[table_idx_cmap]->offset);
    hhea_t = read_table_hhea(buf + tables[table_idx_hhea]->offset);
    hmtx_t = read_table_hmtx(buf + tables[table_idx_hmtx]->offset, 
                maxp_t->glyfs_n, hhea_t->hmetrics_n);

    ttf_log("Head Table\n------------\n");
    ttf_log("Version: %d.%d Rev %d\n", head_t->major_v, 
            head_t->minor_v, head_t->font_rev);
    ttf_log("Magic: 0x%x\n", head_t->magic_n);
    ttf_log("Index2Loc: 0x%x\n", head_t->idx_to_loc_f);

    ttf_log("Maximum Profile Table\n------------\n");
    ttf_log("Version: 0x%x\n", maxp_t->version);
    ttf_log("Num Glyps: %d\n", maxp_t->glyfs_n);
    ttf_log("Max Points: %d\n", maxp_t->max_pts);
    ttf_log("Max Contours: %d\n", maxp_t->max_cont);
    ttf_log("Max Composite Points: %d\n", maxp_t->max_comp_pts);

    ttf_log("CMAP Table\n--------------\n");
    ttf_log("Version: 0x%x\n", cmap_t->version);
    ttf_log("Encodings #: %d\n", cmap_t->encs_n);

    // find correct CMAP encoding to use
    uint32_t cmap_base = tables[table_idx_cmap]->offset;
    for (uint16_t i = 0; i < cmap_t->encs_n; i++) {
        TTF_ENCODING_RECORD* record = cmap_t->encs[i];
        ttf_log("Found Platform Encoding: %d.%d\n", record->platform, record->enc_id);
        uint32_t loc = 0;
        uint16_t format = read_uint16(buf + cmap_base + record->offset, &loc);
        ttf_log("With format: %d\n", format);
        if (format == 4) {
            TTF_CMAP_FMT4* fmt = read_cmap4(buf + cmap_base + record->offset);
            ttf_log("format: %d, len: %d, lan: %d, seg_num: %d, reserved: 0x%x", fmt->format, fmt->len, fmt->lan, fmt->seg_n2 / 2, fmt->reserved);
            cmap_t->encoding = (void*) fmt;
            cmap_t->format = 4;
            break;
        }
    }

    if (cmap_t->encoding == NULL) {
        ttf_error("No supported CMAP encodings found.\n");
        ttf_error("Exiting...\n");
        return;
    }

    // read glyfs into structs
    uint32_t glyf_base = tables[table_idx_glyf]->offset;
    TTF_GLYF** glyfs = ttf_malloc(sizeof(TTF_GLYF*) * maxp_t->glyfs_n);

    for (uint32_t i = 0; i < maxp_t->glyfs_n; i++) {
        TTF_GLYF* glyf = read_glyf(
                buf + glyf_base + loca_t->glyf_offsets[i]);

        if (glyf->cont_n < 0) {
            //ttf_log("Skipping compound Glyf");
            ttf_free(glyf);
            glyfs[i] = NULL;
            continue;
        }

        // this glyf is a space
        if (loca_t->glyf_no_conts[i] == 0xff) {
            glyf->cont_n = 0;
            glyf->x_max = 0;
            glyf->x_min = 0;
            glyf->y_min = 0;
            glyf->y_max = 0;
            glyf->data = NULL;
        }
        else {
            // read Glyph data from addr (2 * 5 = header offset)
            TTF_GLYF_SIMP_D* gdata = read_glyf_simp_d(
                    buf + glyf_base + loca_t->glyf_offsets[i] + sizeof(int16_t) * 5, glyf);
            glyf->data = (void*) gdata;
        }

        if (i < hhea_t->hmetrics_n) {
            glyf->lsb = hmtx_t->hmetrics[i]->lsb;
            glyf->rsb = hmtx_t->hmetrics[i]->advance_w - 
                (glyf->lsb + glyf->x_max - glyf->x_min);
        }
        else {
            glyf->lsb = hmtx_t->rem_lsbs[i - hhea_t->hmetrics_n];
            glyf->rsb = hmtx_t->hmetrics[hhea_t->hmetrics_n - 1]->advance_w - 
                (glyf->lsb + glyf->x_max - glyf->x_min);
        }
        glyfs[i] = glyf;
    }

    ttf_log("Read %d glyfs at %x\n", maxp_t->glyfs_n, glyfs);

    TTF_FONT* font = (TTF_FONT*) ttf_malloc(sizeof(TTF_FONT));

    font->glyfs = glyfs;
    font->glyfs_n = maxp_t->glyfs_n;
    font->head = head_t;
    font->cmap = cmap_t;
    font->free = &free_font;
    *font_out = font;

    for (uint16_t i = 0; i < hhea_t->hmetrics_n; i++) {
        ttf_free(hmtx_t->hmetrics[i]);
    }
    if (hmtx_t->rem_lsbs != NULL) ttf_free(hmtx_t->rem_lsbs);
    ttf_free(hmtx_t);

    ttf_free(loca_t->glyf_offsets);
    ttf_free(loca_t->glyf_no_conts);
    ttf_free(loca_t);

    ttf_free(maxp_t);
    ttf_free(hhea_t);

    for (uint16_t i = 0; i < tables_n; i++) ttf_free(tables[i]);
    ttf_free(tables);
}


TTF_GLYF* glyf_from_char(TTF_FONT* font, uint8_t c) {
    if (font->cmap->format != 4) {
        ttf_error("ERROR Only support type 4 format encoding\n");
        return font->glyfs[0];
    }

    TTF_CMAP_FMT4* fmt = (TTF_CMAP_FMT4*) font->cmap->encoding;
    for (uint16_t i = 0; i < fmt->seg_n2 / 2; i++) {
        if (c >= fmt->start_c[i] && c <= fmt->end_c[i]) {
            uint16_t idx = c + fmt->id_delta[i];
            return font->glyfs[idx];
        }
    }
    return font->glyfs[0];
}

void extract_font_from_file(char* fname, TTF_FONT** font) {
#ifdef USE_STDLIB
    FILE* ttf_file  = fopen(fname, "rb"); 
    if (ttf_file == NULL)
    { 
        ttf_error("File not found\n"); 
        return; 
    }
    int res = fseek(ttf_file, 0, SEEK_END);
    uint32_t size = ftell(ttf_file); 
    uint8_t* buf = ttf_malloc(size); 
    rewind(ttf_file);
    int i = fread(buf, sizeof(uint8_t), size, ttf_file); 
    ttf_log("Read %d", size);
    fclose(ttf_file); 

    extract_font_from_bytes(buf, font);

    ttf_free(buf);
#else
    ttf_error("Cannot use stdio and files with non-stdlib compilation");
#endif
}
