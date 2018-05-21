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

#ifndef __MGOS_ILI9341_H
#define __MGOS_ILI9341_H

#include "mgos.h"
#include "mgos_ili9341_font.h"

// Color definitions for RGB in 565-format
#define ILI9341_BLACK          0x0000   /*   0,   0,   0 */
#define ILI9341_NAVY           0x000F   /*   0,   0, 128 */
#define ILI9341_DARKGREEN      0x03E0   /*   0, 128,   0 */
#define ILI9341_DARKCYAN       0x03EF   /*   0, 128, 128 */
#define ILI9341_MAROON         0x7800   /* 128,   0,   0 */
#define ILI9341_PURPLE         0x780F   /* 128,   0, 128 */
#define ILI9341_OLIVE          0x7BE0   /* 128, 128,   0 */
#define ILI9341_LIGHTGREY      0xC618   /* 192, 192, 192 */
#define ILI9341_DARKGREY       0x7BEF   /* 128, 128, 128 */
#define ILI9341_BLUE           0x001F   /*   0,   0, 255 */
#define ILI9341_GREEN          0x07E0   /*   0, 255,   0 */
#define ILI9341_CYAN           0x07FF   /*   0, 255, 255 */
#define ILI9341_RED            0xF800   /* 255,   0,   0 */
#define ILI9341_MAGENTA        0xF81F   /* 255,   0, 255 */
#define ILI9341_YELLOW         0xFFE0   /* 255, 255,   0 */
#define ILI9341_WHITE          0xFFFF   /* 255, 255, 255 */
#define ILI9341_ORANGE         0xFD20   /* 255, 165,   0 */
#define ILI9341_GREENYELLOW    0xAFE5   /* 173, 255,  47 */
#define ILI9341_PINK           0xF81F

// ILI9341 Display orientation bits, see README.md for details
#define ILI9341_MADCTL_MY    0x80   // LCD Row Order Bottom-Top
#define ILI9341_MADCTL_MX    0x40   // LCD Column Order Right-Left
#define ILI9341_MADCTL_MV    0x20   // LCD Row/Col Reverse
#define ILI9341_MADCTL_ML    0x10   // LCD Vertical Refresh Bottom-Top
#define ILI9341_MADCTL_BGR   0x08   // Blue-Green-Red pixel order
#define ILI9341_MADCTL_MH    0x04   // LCD Horizontal Refresh Right-Left

#define swap(a, b)    { int16_t t = a; a = b; b = t; }

enum mgos_ili9341_rotation_t {
  ILI9341_PORTRAIT       = 0,
  ILI9341_LANDSCAPE      = 1,
  ILI9341_PORTRAIT_FLIP  = 2,
  ILI9341_LANDSCAPE_FLIP = 3,
};

// Externally callable functions:
void mgos_ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void mgos_ili9341_set_fgcolor(uint8_t r, uint8_t g, uint8_t b);
void mgos_ili9341_set_bgcolor(uint8_t r, uint8_t g, uint8_t b);
uint16_t mgos_ili9341_color565(uint8_t r, uint8_t g, uint8_t b);
void mgos_ili9341_set_fgcolor565(uint16_t rgb);
void mgos_ili9341_set_bgcolor565(uint16_t rgb);
void mgos_ili9341_set_orientation(uint8_t madctl, uint16_t width, uint16_t height);
void mgos_ili9341_set_dimensions(uint16_t width, uint16_t height);
void mgos_ili9341_set_rotation(enum mgos_ili9341_rotation_t rotation);
void mgos_ili9341_set_inverted(bool inverted);
uint16_t mgos_ili9341_get_screenWidth();
uint16_t mgos_ili9341_get_screenHeight();

void mgos_ili9341_fillScreen();

// Geometric shapes:
void mgos_ili9341_drawPixel(uint16_t x0, uint16_t y0);
void mgos_ili9341_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void mgos_ili9341_drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
void mgos_ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);

void mgos_ili9341_drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r);
void mgos_ili9341_fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r);

void mgos_ili9341_drawCircle(uint16_t x0, uint16_t y0, uint16_t r);
void mgos_ili9341_fillCircle(uint16_t x0, uint16_t y0, uint16_t r);

void mgos_ili9341_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void mgos_ili9341_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Fonts and Printing:
bool mgos_ili9341_set_font(GFXfont *f);
void mgos_ili9341_print(uint16_t x0, uint16_t y0, const char *s);
void mgos_ili9341_printf(uint16_t x0, uint16_t y0, const char *fmt, ...);
uint16_t mgos_ili9341_getStringWidth(const char *string);
uint16_t mgos_ili9341_getStringHeight(const char *string);
int mgos_ili9341_get_max_font_width(void);
int mgos_ili9341_get_max_font_height(void);

// Returns Y position for line N for the currently selected font.
// If n is negative, returns N-th line from the bottom.
uint16_t mgos_ili9341_line(int n);

// Images
void mgos_ili9341_drawDIF(uint16_t x0, uint16_t y0, char *fn);

#endif // __MGOS_ILI9341_H
