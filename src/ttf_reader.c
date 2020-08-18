#include "ttf_reader.h"

void extract_glyfs(uint8_t* ttf_buffer, TTF_FONT** font_out) {
    uint8_t* buf = ttf_buffer; 
    uint32_t cur = 4;

    uint16_t num_tables = read_uint16(buf, &cur);
    uint16_t search_range = read_uint16(buf, &cur);
    uint16_t entry_selector = read_uint16(buf, &cur);
    uint16_t range_shift = read_uint16(buf, &cur);

    ttf_log("Found 0x%x tables.\n", num_tables); 
    ttf_log("Found 0x%x search range.\n", search_range); 
    ttf_log("Found 0x%x entry selector.\n", entry_selector); 
    ttf_log("Found 0x%x range shift.\n", range_shift); 

    TTF_TABLE** tables = malloc(sizeof(TTF_TABLE*) * num_tables);
    
    TTF_TABLE_HEAD* head_t;
    TTF_TABLE_MAXP* maxp_t;
    TTF_TABLE_LOCA* loca_t;

    uint8_t table_idx_maxp = -1;
    uint8_t table_idx_head = -1;
    uint8_t table_idx_glyf = -1;
    uint8_t table_idx_loca = -1;

    for (uint8_t i = 0; i < num_tables; i++) {
        tables[i] = malloc(sizeof(TTF_TABLE));
        memcpy(tables[i]->tag, buf + cur, 4);
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
            ttf_log("\tand \t\t\t\tINCORRECT checksum!\n", check); 
        }

        if      (strcmp(tables[i]->tag, "head") == 0) table_idx_head = i;
        else if (strcmp(tables[i]->tag, "maxp") == 0) table_idx_maxp = i;
        else if (strcmp(tables[i]->tag, "loca") == 0) table_idx_loca = i;
        else if (strcmp(tables[i]->tag, "glyf") == 0) table_idx_glyf = i;
    }
    
    // Read head table
    head_t = read_table_head(buf + tables[table_idx_head]->offset);
    maxp_t = read_table_maxp(buf + tables[table_idx_maxp]->offset);
    loca_t = read_table_loca(buf + tables[table_idx_loca]->offset, 
            maxp_t->glyfs_n, head_t->idx_to_loc_f);

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


    uint32_t glyf_base = tables[table_idx_glyf]->offset;
    TTF_GLYF** glyfs = malloc(sizeof(TTF_GLYF*) * maxp_t->glyfs_n);

    for (uint32_t i = 0; i < maxp_t->glyfs_n; i++) {
        ttf_log("Found glyph at 0x%x\n", loca_t->glyf_offsets[i]);
        TTF_GLYF* glyf = read_glyf(buf + glyf_base + loca_t->glyf_offsets[i]);
        ttf_log("cont_n: %d\n; x: [%d, %d], y: [%d, %d]\n", glyf->cont_n, 
                glyf->x_min, glyf->x_max, glyf->y_min, glyf->y_max);

        if (glyf->cont_n <= 0) {
            ttf_log("Skipping compound Glyf");
            glyfs[i] = glyf;
            continue;
        }

        // Read Glyph data from addr (2 * 5 = header offset)
        TTF_GLYF_SIMP_D* gdata = read_glyf_simp_d(
                buf + glyf_base + loca_t->glyf_offsets[i] + sizeof(int16_t) * 5, glyf);
        glyf->data = (void*) gdata;

        ttf_log("%d coordinates found\n", gdata->coords_n);
        ttf_log("Strored gdata at 0x%x", gdata);

        glyfs[i] = glyf;
    }
    ttf_log("INside: Found %d glyfs at %x\n", maxp_t->glyfs_n, glyfs);

    TTF_FONT* font = (TTF_FONT*) malloc(sizeof(TTF_FONT));
    font->glyfs = glyfs;
    font->glyfs_n = maxp_t->glyfs_n;
    font->head = head_t;

    *font_out = font;
}

void extract_font_from_file(char* fname, TTF_FONT** font) {
    FILE* ttf_file  = fopen(fname, "rb"); 
    if (ttf_file == NULL)
    { 
        printf("File not found\n"); 
        return; 
    }
    int res = fseek(ttf_file, 0, SEEK_END);
    uint32_t size = ftell(ttf_file); 
    uint8_t* buf = malloc(size); 
    rewind(ttf_file);
    int i = fread(buf, sizeof(uint8_t), size, ttf_file); 
    ttf_log("Read %d", size);
    fclose(ttf_file); 

    extract_glyfs(buf, font);
}

