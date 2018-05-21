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

#ifndef __MGOS_ILI9341_FONT_H
#define __MGOS_ILI9341_FONT_H

#include "mgos.h"
#include "mgos_ili9341.h"
#include "gfxfont.h"

// Internal functions -- do not use
uint16_t ili9341_print_fillPixelLine(const char *string, uint8_t line, uint16_t *buf, uint16_t color);

#endif // __MGOS_ILI9341_FONT_H
