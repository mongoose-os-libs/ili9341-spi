/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* TODO: High-level file comment. */

/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* TODO: High-level file comment. */

#include "mgos_ili9341.h"

/* These functions were borrowed from Adafruit's excellent Arduino Graphics library:
 * https://github.com/adafruit/Adafruit-GFX-Library/
 *
 * Copyright (c) 2012 Adafruit Industries.  All rights reserved.
 */

static void ili9341_drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x1) {
      mgos_ili9341_drawPixel(x0 - y, y0 - x);
      mgos_ili9341_drawPixel(x0 - x, y0 - y);
    }
    if (cornername & 0x2) {
      mgos_ili9341_drawPixel(x0 + x, y0 - y);
      mgos_ili9341_drawPixel(x0 + y, y0 - x);
    }
    if (cornername & 0x4) {
      mgos_ili9341_drawPixel(x0 + x, y0 + y);
      mgos_ili9341_drawPixel(x0 + y, y0 + x);
    }
    if (cornername & 0x8) {
      mgos_ili9341_drawPixel(x0 - y, y0 + x);
      mgos_ili9341_drawPixel(x0 - x, y0 + y);
    }
  }
}

static void ili9341_fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, int16_t delta) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t ylm = x0 - r;

  while (x < y) {
    if (f >= 0) {
      if (cornername & 0x1) {
        mgos_ili9341_drawLine(x0+y>0?x0+y:0, y0-x>0?y0-x:0, x0+y>0?x0+y:0, y0-x+2*x+1+delta);
      }
      if (cornername & 0x2) {
        mgos_ili9341_drawLine(x0-y>0?x0-y:0, y0-x>0?y0-x:0, x0-y>0?x0-y:0, y0-x+2*x+1+delta);
      }
      ylm = x0 - y;
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if ((x0 - x) > ylm) {
      if (cornername & 0x1) {
        mgos_ili9341_drawLine(x0+x>0?x0+x:0, y0-y>0?y0-y:0, x0+x>0?x0+x:0, y0-y+2*y+1+delta);
      }
      if (cornername & 0x2) {
        mgos_ili9341_drawLine(x0-x>0?x0-x:0, y0-y>0?y0-y:0, x0-x>0?x0-x:0, y0-y+2*y+1+delta);
      }
    }
  }
}

void mgos_ili9341_drawCircle(uint16_t x, uint16_t y, uint16_t r) {
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x1 = 0;
  int y1 = r;

  mgos_ili9341_drawPixel(x, y + r);
  mgos_ili9341_drawPixel(x, y - r);
  mgos_ili9341_drawPixel(x + r, y);
  mgos_ili9341_drawPixel(x - r, y);
  while(x1 < y1) {
    if (f >= 0) {
      y1--;
      ddF_y += 2;
      f += ddF_y;
    }
    x1++;
    ddF_x += 2;
    f += ddF_x;
    mgos_ili9341_drawPixel(x + x1, y + y1);
    mgos_ili9341_drawPixel(x - x1, y + y1);
    mgos_ili9341_drawPixel(x + x1, y - y1);
    mgos_ili9341_drawPixel(x - x1, y - y1);
    mgos_ili9341_drawPixel(x + y1, y + x1);
    mgos_ili9341_drawPixel(x - y1, y + x1);
    mgos_ili9341_drawPixel(x + y1, y - x1);
    mgos_ili9341_drawPixel(x - y1, y - x1);
  }
}

void mgos_ili9341_fillCircle(uint16_t x0, uint16_t y0, uint16_t r) {
  mgos_ili9341_drawLine(x0, y0-r>0?y0-r:0, x0, y0+r+1);
  ili9341_fillCircleHelper(x0, y0, r, 3, 0);
}


void mgos_ili9341_drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  mgos_ili9341_drawLine(x0, y0, x0+w-1, y0);
  mgos_ili9341_drawLine(x0+w-1, y0, x0+w-1, y0+h-1);
  mgos_ili9341_drawLine(x0, y0+h-1, x0+w-1, y0+h-1);
  mgos_ili9341_drawLine(x0, y0, x0, y0+h-1);
}

void mgos_ili9341_drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r) {
  // draw the straight edges
  mgos_ili9341_drawLine(x0+r, y0, x0+w-r, y0);         // Top
  mgos_ili9341_drawLine(x0+r, y0+h-1, x0+w-r, y0+h-1); // Bottom
  mgos_ili9341_drawLine(x0, y0+r, x0, y0+h-r);         // Left
  mgos_ili9341_drawLine(x0+w-1, y0+r, x0+w-1, y0+h-r); // Right

  // draw four corners
  ili9341_drawCircleHelper(x0+r, y0+r, r, 1);          // Top Left
  ili9341_drawCircleHelper(x0+w-r-1, y0+r, r, 2);      // Top Right
  ili9341_drawCircleHelper(x0+r, y0+h-r-1, r, 8);      // Bottom Left
  ili9341_drawCircleHelper(x0+w-r-1, y0+h-r-1, r, 4);  // Bottom Right
}

void mgos_ili9341_fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r) {
  mgos_ili9341_fillRect(x0+r, y0, w-2*r, h);
  ili9341_fillCircleHelper(x0+w-r-1, y0+r, r, 1, h-2*r-1);
  ili9341_fillCircleHelper(x0+r, y0+r, r, 2, h-2*r-1);
}

void mgos_ili9341_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  mgos_ili9341_drawLine(x0, y0, x1, y1);
  mgos_ili9341_drawLine(x1, y1, x2, y2);
  mgos_ili9341_drawLine(x2, y2, x0, y0);
}

void mgos_ili9341_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    mgos_ili9341_drawLine(a, y0, b+1, y0);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    mgos_ili9341_drawLine(a, y, b+1, y);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    mgos_ili9341_drawLine(a, y, b+1, y);
  }
}
