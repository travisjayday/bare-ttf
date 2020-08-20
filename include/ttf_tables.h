#ifndef TTF_TABLES_H
#define TTF_TABLES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Glyf flag bitmasks 
#define F_ON_CURVE_POINT 0x01
#define F_X_SHORT_VECTOR 0x02
#define F_Y_SHORT_VECTOR 0x04
#define F_REPEAT_FLAG 0x08
#define F_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR 0x10
#define F_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR 0x20
#define F_OVERLAP_SIMPLE 0x40
#define F_IS_ENDPOINT 0x80

typedef struct {
    char tag[5];
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} TTF_TABLE;

typedef struct {
    uint16_t major_v;
    uint16_t minor_v;
    uint32_t font_rev;
    uint32_t check_sum_adj;
    uint32_t magic_n;
    uint16_t flags;
    uint16_t units_per_em;
    uint64_t created_t;
    uint64_t modified_t;
    uint16_t x_min;
    uint16_t y_min;
    uint16_t x_max;
    uint16_t y_max;
    uint16_t mac_style;
    uint16_t lowest_rec_ppem;
    uint16_t font_dir_h;
    uint16_t idx_to_loc_f;
    uint16_t glyf_data_f; 
} TTF_TABLE_HEAD;

typedef struct {
    uint32_t version;
    uint16_t glyfs_n; 
    uint16_t max_pts;
    uint16_t max_cont;
    uint16_t max_comp_pts;
    uint16_t max_zones;
    uint16_t max_twi_pts;
    uint16_t max_storage;
    uint16_t max_func_defs;
    uint16_t max_instr_defs;
    uint16_t max_stack_elms;
    uint16_t max_instr_size;
    uint16_t max_comp_elms;
    uint16_t max_comp_depth;
} TTF_TABLE_MAXP;

typedef struct {
    uint16_t major_v;
    uint16_t minor_v;
    int16_t ascender;
    int16_t descender;
    int16_t line_gap;
    uint16_t max_adv_w;
    int16_t min_lsb;
    int16_t min_rsb;
    int16_t max_x_ext;
    int16_t caret_slopri;
    int16_t caret_slopru;
    int16_t caret_offset;
    uint32_t res12;
    uint32_t res34;
    int16_t fmt_metric_d;
    uint16_t hmetrics_n;
} TTF_TABLE_HHEA;

typedef struct {
    uint16_t advance_w;
    int16_t lsb;
} _HMTX_RECORD;

typedef struct {
    _HMTX_RECORD** hmetrics;
    int16_t* rem_lsbs;
} TTF_TABLE_HMTX;

typedef struct {
    uint32_t* glyf_offsets;
    uint8_t* glyf_no_conts; 
} TTF_TABLE_LOCA;

typedef struct {
    int16_t cont_n;
    int16_t x_min;
    int16_t y_min;
    int16_t x_max;
    int16_t y_max;
    int16_t lsb; 
    int16_t rsb;

/* if cont_n < 0, data points to a TTF_COMP_GLYPH, else 
it points to a TTF_SIMP_GLYPH */ 
    void* data;
} TTF_GLYF;

typedef struct {
    uint16_t cont_n;
    uint16_t* cont_endpts;
    uint16_t instr_n;
/* NULL if no instructions */
    uint8_t* instr;                     
    uint8_t* flags;
    uint16_t coords_n;
    int16_t* x_coords; 
    int16_t* y_coords; 
} TTF_GLYF_SIMP_D;

typedef struct {

} TTF_GLYF_COMP_D;

typedef struct {
    uint16_t platform;
    uint16_t enc_id;
    uint32_t offset;
} TTF_ENCODING_RECORD;

typedef struct {
    uint16_t format;
    uint16_t len;
    uint16_t lan;
    uint16_t seg_n2;
    uint16_t search_rng;
    uint16_t entry_sel;
    uint16_t range_shift;
    uint16_t* end_c;
    uint16_t reserved;
    uint16_t* start_c;
    int16_t* id_delta;
    uint16_t* id_rng_offset;
    uint16_t  id_arr_n; 
    uint16_t* id_arr;
} TTF_CMAP_FMT4;

typedef struct {
    uint16_t version;
    uint16_t encs_n;
    TTF_ENCODING_RECORD** encs; 
    uint8_t format;
    // void* depends on format. if format == 4, encoding points to the first TTF_CMAP_FMT4 struct
    void* encoding; 
} TTF_TABLE_CMAP;
 


TTF_TABLE_HEAD* read_table_head(uint8_t* loc);
TTF_TABLE_CMAP* read_table_cmap(uint8_t* loc);
TTF_TABLE_MAXP* read_table_maxp(uint8_t* loc);
TTF_TABLE_LOCA* read_table_loca(uint8_t* loc, 
        uint32_t glyfs_n, uint8_t idx_to_loc_f);
TTF_TABLE_HHEA* read_table_hhea(uint8_t* loc);
TTF_TABLE_HMTX* read_table_hmtx(uint8_t* loc, 
        uint16_t glyfs_n, uint16_t hmetrics_n);

TTF_CMAP_FMT4* read_cmap4(uint8_t* loc);
TTF_GLYF* read_glyf(uint8_t* loc);
TTF_GLYF_SIMP_D* read_glyf_simp_d(uint8_t* loc, TTF_GLYF* header);


#endif
