## The jankest .TTF font parser and renderer in town
**Please do not use this in production**. This project only implements the bare minimum of the OTF spec to support very basic font parsing and rendering. It has been tested mainly on the Ubuntu fontface (`ubuntu.ttf`) and no guarantees are made that it will render other fonts correctly. 

On a lighter note, this project has no dependencies and can be easily adapted to run on any system. 

## Usage
```
    /* 1. Load a TTF font */
    TTF_FONT* font = NULL;

        // either from disk 
        extract_font_from_file("ubuntu.ttf", &font);

        // or from a bytes buffer
        uint8_t* buffer; 
        extract_font_from_bytes(buffer, &font);

    /* 2. Get a Glyph from a char c */ 
    TTF_GLYF* glyf = glyf_from_char(font, c); 

    /* 3. Rasterize the Glyph to a buffer (size is in pt) */
    GLYF_PIXBUF pixbuf = rasterize_glyf(glyf, font->head->units_per_em / size);

    /* 4. Draw the pixel buffer using */
    pixbuf->w   // width of pixel buffer
    pixbuf->h   // height of pixel buffer
    pixbuf->buf // data (uint8_t*) of pixel buffer. 0xff -> white, 0x0 -> black / no color
```
