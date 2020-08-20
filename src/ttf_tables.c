#include "ttf_tables.h"
#include "ttf_utils.h"

TTF_TABLE_HEAD* read_table_head(uint8_t* loc) {
   uint32_t cur = 0; 
   TTF_TABLE_HEAD* table = malloc(sizeof(TTF_TABLE_HEAD));

   table->major_v               = read_uint16(loc, &cur);
   table->minor_v               = read_uint16(loc, &cur);
   table->font_rev              = read_uint32(loc, &cur);
   table->check_sum_adj         = read_uint32(loc, &cur);
   table->magic_n               = read_uint32(loc, &cur);
   table->flags                 = read_uint16(loc, &cur);
   table->units_per_em          = read_uint16(loc, &cur);
   table->created_t             = read_uint64(loc, &cur);
   table->modified_t            = read_uint64(loc, &cur);
   table->x_min                 = read_uint16(loc, &cur);
   table->y_min                 = read_uint16(loc, &cur);
   table->x_max                 = read_uint16(loc, &cur);
   table->y_max                 = read_uint16(loc, &cur);
   table->mac_style             = read_uint16(loc, &cur);
   table->lowest_rec_ppem       = read_uint16(loc, &cur);
   table->font_dir_h            = read_uint16(loc, &cur);
   table->idx_to_loc_f          = read_uint16(loc, &cur);
   table->glyf_data_f           = read_uint16(loc, &cur);

   return table;
}

TTF_TABLE_MAXP* read_table_maxp(uint8_t* loc) {
    uint32_t cur = 0;
    TTF_TABLE_MAXP* table = calloc(1, sizeof(TTF_TABLE_MAXP));

    table->version              = read_uint32(loc, &cur);
    table->glyfs_n              = read_uint16(loc, &cur);

    // maxp table is version 1.0
    if (table->version == 0x00010000) {
        table->max_pts          = read_uint16(loc, &cur);
        table->max_cont         = read_uint16(loc, &cur);
        table->max_comp_pts     = read_uint16(loc, &cur);
        table->max_zones        = read_uint16(loc, &cur);
        table->max_twi_pts      = read_uint16(loc, &cur);
        table->max_storage      = read_uint16(loc, &cur);
        table->max_func_defs    = read_uint16(loc, &cur);
        table->max_instr_defs   = read_uint16(loc, &cur);
        table->max_stack_elms   = read_uint16(loc, &cur);
        table->max_instr_size   = read_uint16(loc, &cur);
        table->max_comp_elms    = read_uint16(loc, &cur);
        table->max_comp_depth   = read_uint16(loc, &cur);
    }
    else {
        printf("DO NOT SUPPORT NON V1 MAXP");
    }

    return table;
}

TTF_TABLE_HHEA* read_table_hhea(uint8_t* loc) {
    uint32_t cur = 0;
    TTF_TABLE_HHEA* table = calloc(1, sizeof(TTF_TABLE_HHEA));

    table->major_v      = read_uint16(loc, &cur);
    table->minor_v      = read_uint16(loc, &cur);
    table->ascender     = read_int16(loc, &cur);
    table->descender    = read_int16(loc, &cur);
    table->line_gap     = read_int16(loc, &cur);
    table->max_adv_w    = read_uint16(loc, &cur);
    table->min_lsb      = read_int16(loc, &cur);
    table->min_rsb      = read_int16(loc, &cur);
    table->max_x_ext    = read_int16(loc, &cur);
    table->caret_slopri = read_int16(loc, &cur);
    table->caret_slopru = read_int16(loc, &cur);
    table->caret_offset = read_int16(loc, &cur);
    table->res12        = read_uint32(loc, &cur);
    table->res34        = read_uint32(loc, &cur);
    table->fmt_metric_d = read_int16(loc, &cur);
    table->hmetrics_n   = read_uint16(loc, &cur);

    return table;
}

TTF_TABLE_HMTX* read_table_hmtx(uint8_t* loc, uint16_t glyfs_n, uint16_t hmetrics_n) {
    uint32_t cur = 0;
    TTF_TABLE_HMTX* table = calloc(1, sizeof(TTF_TABLE_HMTX));

    table->hmetrics = (_HMTX_RECORD**) malloc(
            hmetrics_n * sizeof(_HMTX_RECORD*));
    for (uint16_t i = 0; i < hmetrics_n; i++) {
        _HMTX_RECORD* rec = malloc(sizeof(_HMTX_RECORD));
        rec->advance_w = read_uint16(loc, &cur);
        rec->lsb = read_int16(loc, &cur);
        table->hmetrics[i] = rec;
    }

    uint16_t rem = glyfs_n - hmetrics_n;
    if (rem > 0) {
        table->rem_lsbs = (int16_t*) malloc(
               rem * sizeof(int16_t)); 
        for (uint16_t i = 0; i < rem; i++) 
            table->rem_lsbs[i] = read_int16(loc, &cur); 
    }
    else 
        table->rem_lsbs = NULL;

    return table;
}

TTF_TABLE_LOCA* read_table_loca(uint8_t* loc, uint32_t glyfs_n, 
        uint8_t idx_to_loc_f) {
    uint32_t cur = 0;
    TTF_TABLE_LOCA* table = calloc(1, sizeof(TTF_TABLE_LOCA));

    table->glyf_offsets = (uint32_t*) malloc(glyfs_n * sizeof(uint32_t));
    table->glyf_no_conts = (uint8_t*) calloc(glyfs_n, sizeof(uint8_t));

    if (idx_to_loc_f == 1) {
        // uses long offsets (Offset32)
        for (uint32_t i = 0; i < glyfs_n; i++) {
            uint32_t d = read_uint32(loc, &cur);
            if (i < glyfs_n - 1 && d == read_uint32(loc, &cur)) 
                table->glyf_no_conts[i] = 0xff;
            table->glyf_offsets[i] = d;
            cur -= sizeof(uint32_t);
        }
    }
    else {
        // uses short offsets (Offset16)
        for (uint32_t i = 0; i < glyfs_n; i++) {
            uint32_t d = read_uint16(loc, &cur);
            if (i < glyfs_n - 1 && d == read_uint16(loc, &cur)) 
                table->glyf_no_conts[i] = 0xff;
            table->glyf_offsets[i] = 2 * d;
            cur -= sizeof(uint16_t);
        }
    }
    return table;
}

TTF_TABLE_CMAP* read_table_cmap(uint8_t* loc) {
    uint32_t cur = 0;
    TTF_TABLE_CMAP* table = calloc(1, sizeof(TTF_TABLE_CMAP));
    table->version = read_uint16(loc, &cur);
    table->encs_n = read_uint16(loc, &cur);

    table->encs = (TTF_ENCODING_RECORD**) malloc(
        table->encs_n * sizeof(TTF_ENCODING_RECORD*));

    for (uint16_t i = 0; i < table->encs_n; i++) {
        TTF_ENCODING_RECORD* record = (TTF_ENCODING_RECORD*) malloc(
            sizeof(TTF_ENCODING_RECORD));
        record->platform = read_uint16(loc, &cur);
        record->enc_id = read_uint16(loc, &cur);
        record->offset = read_uint32(loc, &cur);
        printf("Found record at 0x%x", record->offset);
        table->encs[i] = record;
    }

    return table;
}

TTF_CMAP_FMT4* read_cmap4(uint8_t* loc) {
    TTF_CMAP_FMT4* fmt = malloc(sizeof(TTF_CMAP_FMT4));
    uint32_t cur = 0;
    fmt->format         = read_uint16(loc, &cur);
    fmt->len            = read_uint16(loc, &cur);
    fmt->lan            = read_uint16(loc, &cur);
    fmt->seg_n2         = read_uint16(loc, &cur);
    fmt->search_rng     = read_uint16(loc, &cur);
    fmt->entry_sel      = read_uint16(loc, &cur);
    fmt->range_shift    = read_uint16(loc, &cur);

    uint32_t seg_n      = fmt->seg_n2 / 2;

    fmt->end_c = (uint16_t*) malloc(seg_n * sizeof(uint16_t));
    for (uint32_t i = 0; i < seg_n; i++) 
        fmt->end_c[i] = read_uint16(loc, &cur);

    fmt->reserved       = read_uint16(loc, &cur);

    fmt->start_c = (uint16_t*) malloc(seg_n * sizeof(uint16_t));
    for (uint32_t i = 0; i < seg_n; i++) 
        fmt->start_c[i] = read_uint16(loc, &cur);

    fmt->id_delta = (int16_t*) malloc(seg_n * sizeof(int16_t));
    for (uint32_t i = 0; i < seg_n; i++) 
        fmt->id_delta[i] = read_int16(loc, &cur);

    fmt->id_rng_offset = (uint16_t*) malloc(seg_n * sizeof(uint16_t));
    for (uint32_t i = 0; i < seg_n; i++) 
        fmt->id_rng_offset[i] = read_uint16(loc, &cur);
    
    fmt->id_arr_n = fmt->len - cur;
    fmt->id_arr = (uint16_t*) malloc(fmt->id_arr_n * sizeof(uint16_t)); 
    for (uint16_t i = 0; i < fmt->id_arr_n; i++) {
        fmt->id_arr[i] = read_uint16(loc, &cur);
    }

    return fmt;
}

TTF_GLYF* read_glyf(uint8_t* loc) {
    uint32_t cur = 0;
    TTF_GLYF* table = malloc(sizeof(TTF_GLYF));
    table->cont_n = read_int16(loc, &cur);
    table->x_min = read_int16(loc, &cur);
    table->y_min = read_int16(loc, &cur);
    table->x_max = read_int16(loc, &cur);
    table->y_max = read_int16(loc, &cur);
    return table;
}

void read_coords(TTF_GLYF_SIMP_D* gdata, 
        int16_t* coords_ptr, uint8_t* loc, uint32_t* cur,
        uint8_t IS_SHORT, uint8_t IS_SAME_OR_POS) {
    int16_t coord = 0; 
    int16_t l_cont_idx = 0; 
    for (uint16_t i = 0; i < gdata->coords_n; i++) {
        if (gdata->flags[i] & IS_SHORT) {
            // delta coord is 1 byte long, 
            if (gdata->flags[i] & IS_SAME_OR_POS) 
                // delta coord is positive    
                coord += (int16_t) read_uint8(loc, cur); 
            else 
                // delta coord is negative
                coord -= (int16_t) read_uint8(loc, cur); 
        }
        else {
            if (gdata->flags[i] & IS_SAME_OR_POS) {}
                // coord is same as previous coord
            else 
                // delta coord is signed 2b int 
                coord += read_int16(loc, cur);
        }
        for (uint16_t j = l_cont_idx; j < gdata->cont_n 
                && gdata->cont_endpts[j] <= i; j++) {
            if (gdata->cont_endpts[j] == i) {
               gdata->flags[i] |= F_IS_ENDPOINT;
               l_cont_idx = j;
            }
        }
        coords_ptr[i] = coord;
    }
}

TTF_GLYF_SIMP_D* read_glyf_simp_d(uint8_t* loc, TTF_GLYF* header) {
    uint32_t cur = 0;

    TTF_GLYF_SIMP_D* gdata = (TTF_GLYF_SIMP_D*) malloc(
            sizeof(TTF_GLYF_SIMP_D));

    // read contour endpoint indices and find max idx
    gdata->cont_endpts = (uint16_t*) malloc(
            header->cont_n * sizeof(uint16_t));
    uint32_t max_idx = 0;
    for (uint16_t i = 0; i < header->cont_n; i++) {
        gdata->cont_endpts[i] = read_uint16(loc, &cur); 
        if (gdata->cont_endpts[i] > max_idx)
            max_idx = gdata->cont_endpts[i];
    }
    gdata->cont_n = header->cont_n;
    gdata->coords_n = max_idx + 1;

    printf("%d COORD_NX. cont_n%d, ", gdata->coords_n, header->cont_n);
    fflush(stdout);

    // read bytecode instructions
    gdata->instr_n = read_uint16(loc, &cur);
    gdata->instr = NULL;
    if (gdata->instr_n > 0) {
        gdata->instr = (uint8_t*) malloc(
                gdata->instr_n * sizeof(uint8_t));
        for (uint16_t i = 0; i < gdata->instr_n; i++) 
            gdata->instr[i] = read_uint8(loc, &cur);
    }
    
    gdata->flags = (uint8_t*) malloc((gdata->coords_n) * sizeof(uint8_t));

    // populate flags 
    for (uint16_t i = 0; i < gdata->coords_n; ) {
        uint8_t flag = read_uint8(loc, &cur);
        uint8_t reps = 1;
        if (flag & F_REPEAT_FLAG) reps += read_uint8(loc, &cur);
        while (reps-- > 0) gdata->flags[i++] = flag;
    }

    gdata->x_coords = (int16_t*) malloc(
        gdata->coords_n * sizeof(int16_t));
    gdata->y_coords = (int16_t*) malloc(
        gdata->coords_n * sizeof(int16_t));

    read_coords(gdata, gdata->x_coords, loc, &cur,
            F_X_SHORT_VECTOR, F_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR);
    read_coords(gdata, gdata->y_coords, loc, &cur,
            F_Y_SHORT_VECTOR, F_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR);

    return gdata;
}
