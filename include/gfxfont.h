#ifndef _GFXFONT_H_
#define _GFXFONT_H_

typedef struct { // Data stored PER GLYPH
  uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
  uint8_t  width, height;    // Bitmap dimensions in pixels
  uint8_t  xAdvance;         // Distance to advance cursor (x axis)
  int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
  uint8_t  *bitmap;      // Glyph bitmaps, concatenated
  GFXglyph *glyph;       // Glyph array
  uint8_t   first, last; // ASCII extents
  uint8_t   yAdvance;    // Newline distance (y axis)
      
  // Added(pimvanpelt) for framebuffer rendering.
  int8_t    font_height; // Maximum per-glyph height
  int8_t    font_width;  // Maximum per-glyph width
  int8_t    font_min_xOffset;  // Left-most glyph xOffset
  int8_t    font_min_yOffset;  // Left-most glyph yOffset
} GFXfont;

enum GFXfont_t {
  GFXFONT_NONE      =0,
  GFXFONT_INTERNAL  =1,
  GFXFONT_FILE      =2,
};

#endif // _GFXFONT_H_
