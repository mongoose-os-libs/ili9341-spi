/*
 * Copyright (c) 2017, 2012 Adafruit Industries
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

/* Modified by Pim van Pelt <pim@google.com> */

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

typedef struct {             // Data stored PER GLYPH
  uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
  uint8_t  width, height;    // Bitmap dimensions in pixels
  uint8_t  xAdvance;         // Distance to advance cursor (x axis)
  int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct {         // Data stored for FONT AS A WHOLE:
  uint8_t * bitmap;      // Glyph bitmaps, concatenated
  GFXglyph *glyph;       // Glyph array
  uint8_t   first, last; // ASCII extents
  uint8_t   yAdvance;    // Newline distance (y axis)

  // Added(pimvanpelt) for framebuffer rendering.
  int8_t    font_height;      // Maximum per-glyph height
  int8_t    font_width;       // Maximum per-glyph width
  int8_t    font_min_xOffset; // Left-most glyph xOffset
  int8_t    font_min_yOffset; // Left-most glyph yOffset
} GFXfont;

enum GFXfont_t {
  GFXFONT_NONE     =0,
  GFXFONT_INTERNAL =1,
  GFXFONT_FILE     =2,
};

#endif // _GFXFONT_H_
