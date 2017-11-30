#ifndef __MGOS_ILI9341_FONT_H
#define __MGOS_ILI9341_FONT_H

#include "mgos.h"
#include "mgos_ili9341.h"
#include "gfxfont.h"

// Internal functions -- do not use
uint16_t ili9341_print_fillPixelLine(char *string, uint8_t line, uint16_t *buf, uint16_t color);

#endif // __MGOS_ILI9341_FONT_H
