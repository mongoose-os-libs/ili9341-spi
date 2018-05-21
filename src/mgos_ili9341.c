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

#include "mgos_ili9341.h"

#include <unistd.h>

#include "common/str_util.h"

#include "mgos_config.h"
#include "mgos_gpio.h"

#include "mgos_ili9341_hal.h"
#include "mgos_ili9341_font.h"

#define SPI_MODE    0

struct ili9341_window {
  uint16_t x0;
  uint16_t x1;
  uint16_t y0;
  uint16_t y1;
  uint16_t fg_color; // in network byte order
  uint16_t bg_color; // in network byte order
};

static uint16_t s_screen_width;
static uint16_t s_screen_height;
static struct ili9341_window s_window;

static const uint8_t ILI9341_init[] = {
  ILI9341_SWRESET,   ILI9341_DELAY, 5,    //  1: Software reset, no args, w/ 5 ms delay afterwards
  ILI9341_POWERA,    5,             0x39, 0x2c, 0x00, 0x34, 0x02,
  ILI9341_POWERB,    3,             0x00, 0xc1, 0x30,
  0xef,              3,             0x03, 0x80, 0x02,
  ILI9341_DTCA,      3,             0x85, 0x00, 0x78,
  ILI9341_DTCB,      2,             0x00, 0x00,
  ILI9341_POWER_SEQ, 4,             0x64, 0x03, 0x12, 0x81,
  ILI9341_PRC,       1,             0x20,
  ILI9341_PWCTR1,    1,             0x23,                          // Power control VRH[5:0]
  ILI9341_PWCTR2,    1,             0x10,                          // Power control SAP[2:0];BT[3:0]
  ILI9341_VMCTR1,    2,             0x3e, 0x28,                    // VCM control
  ILI9341_VMCTR2,    1,             0x86,                          // VCM control2
  ILI9341_MADCTL,    1,             (MADCTL_MX | ILI9341_RGB_BGR), // Memory Access Control (orientation)
  // *** INTERFACE PIXEL FORMAT: 0x66 -> 18 bit; 0x55 -> 16 bit
  ILI9341_PIXFMT,    1,             0x55,
  ILI9341_INVOFF,    0,
  ILI9341_FRMCTR1,   2,             0x00, 0x13,             // 0x18 79Hz, 0x1B default 70Hz, 0x13 100Hz
  ILI9341_DFUNCTR,   4,             0x08, 0x82, 0x27, 0x00, // Display Function Control
  ILI9341_PTLAR,     4,             0x00, 0x00, 0x01, 0x3F,
  ILI9341_3GAMMA_EN, 1,             0x00,                   // 3Gamma Function: Disable (0x02), Enable (0x03)
  ILI9341_GAMMASET,  1,             0x01,                   // Gamma curve selected (0x01, 0x02, 0x04, 0x08)
  ILI9341_GMCTRP1,   15,                                    // Positive Gamma Correction
  0x0F,              0x31,          0x2B, 0x0C, 0x0E, 0x08, 0x4E,0xF1,  0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  ILI9341_GMCTRN1,   15 | ILI9341_DELAY,                    // Negative Gamma Correction
  0x00,              0x0E,          0x14, 0x03, 0x11, 0x07, 0x31,0xC1,  0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  120,                                                      // 120 ms delay before sleepout
  ILI9341_SLPOUT,    0,                                     // Sleep out
  ILI9341_DISPON,    0,
  ILI9341_INVALID_CMD,                                      // End of sequence.
};


// SPI -- Hardware Interface, function names start with ili9341_spi_
// and are all declared static.
static void ili9341_spi_write(const uint8_t *data, uint32_t size) {
  struct mgos_spi *spi = mgos_spi_get_global();

  if (!spi) {
    LOG(LL_ERROR, ("SPI is disabled, set spi.enable=true"));
    return;
  }

  struct mgos_spi_txn txn = {
    .cs   = mgos_sys_config_get_ili9341_cs_index(),
    .mode = SPI_MODE,
    .freq = mgos_sys_config_get_ili9341_spi_freq(),
  };
  txn.hd.tx_data   = data,
  txn.hd.tx_len    = size,
  txn.hd.dummy_len = 0,
  txn.hd.rx_len    = 0,
  txn.hd.rx_data   = NULL,
  mgos_spi_run_txn(spi, false, &txn);
}

static void ili9341_spi_write8_cmd(uint8_t byte) {
  // Command has DC low and CS low while writing to SPI bus.
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);
  ili9341_spi_write(&byte, 1);
}

static void ili9341_spi_write8(uint8_t byte) {
  // Data has DC high and CS low while writing to SPI bus.
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  ili9341_spi_write(&byte, 1);
}

// ILI9341 Primitives -- these methods call SPI commands directly,
// start with ili9341_ and are all declared static.
static void ili9341_commandList(const uint8_t *addr) {
  uint8_t numArgs, cmd, delay;

  while (true) {                                        // For each command...
    cmd = *addr++;                                      // save command
    // 0 is a NOP, so technically a valid command, but why on earth would one want it in the list
    if (cmd == ILI9341_INVALID_CMD) {
      break;
    }
    numArgs  = *addr++;                                 // Number of args to follow
    delay    = numArgs & ILI9341_DELAY;                 // If high bit set, delay follows args
    numArgs &= ~ILI9341_DELAY;                          // Mask out delay bit

    mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);
    ili9341_spi_write(&cmd, 1);

    mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
    ili9341_spi_write((uint8_t *)addr, numArgs);
    addr += numArgs;

    if (delay) {
      delay = *addr++;               // Read post-command delay time (ms)
      mgos_msleep(delay);
    }
  }
}

static void ili9341_set_clip(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  ili9341_spi_write8_cmd(ILI9341_CASET); // Column addr set
  ili9341_spi_write8(x0 >> 8);
  ili9341_spi_write8(x0 & 0xFF);         // XSTART
  ili9341_spi_write8(x1 >> 8);
  ili9341_spi_write8(x1 & 0xFF);         // XEND
  ili9341_spi_write8_cmd(ILI9341_PASET); // Row addr set
  ili9341_spi_write8(y0 >> 8);
  ili9341_spi_write8(y0);                // YSTART
  ili9341_spi_write8(y1 >> 8);
  ili9341_spi_write8(y1);                // YEND
  ili9341_spi_write8_cmd(ILI9341_RAMWR); // write to RAM
  return;
}

// buf represents a 16-bit RGB 565 uint16_t color buffer of length buflen bytes (so buflen/2 pixels).
// Note: data in 'buf' has to be in network byte order!
static void ili9341_send_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *buf, uint32_t buflen) {
  uint16_t winsize = (x1 - x0 + 1) * (y1 - y0 + 1);

  if (y0 + s_window.y0 > s_window.y1 || x0 + s_window.x0 > s_window.x1) {
    return;
  }
  if (y1 + s_window.y0 > s_window.y1) {
    y1 = s_window.y1 - s_window.y0;
  }
  if (x1 + s_window.x0 > s_window.x1) {
    x1 = s_window.x1 - s_window.x0;
  }

  if (buflen != winsize * 2) {
    LOG(LL_ERROR, ("Want buflen(%d), to be twice the window size(%d)", buflen, winsize));
    return;
  }

  winsize = (x1 - x0 + 1) * (y1 - y0 + 1);

  ili9341_set_clip(x0 + s_window.x0, y0 + s_window.y0, x1 + s_window.x0, y1 + s_window.y0);
  ili9341_spi_write8_cmd(ILI9341_RAMWR);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  ili9341_spi_write(buf, winsize * 2);
}

#define ILI9341_FILLRECT_CHUNK    256
static void ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  uint16_t *buf;
  uint32_t  i;
  uint32_t  todo_len;
  uint32_t  buflen;

  todo_len = w * h;
  if (todo_len == 0) {
    return;
  }

  // Allocate at most 2*FILLRECT_CHUNK bytes
  buflen = (todo_len < ILI9341_FILLRECT_CHUNK ? todo_len : ILI9341_FILLRECT_CHUNK);

  if (!(buf = malloc(buflen * sizeof(uint16_t)))) {
    return;
  }

  for (i = 0; i < buflen; i++) {
    buf[i] = s_window.fg_color;
  }

  ili9341_set_clip(x0, y0, x0 + w - 1, y0 + h - 1);
  ili9341_spi_write8_cmd(ILI9341_RAMWR);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  while (todo_len) {
    if (todo_len >= buflen) {
      ili9341_spi_write((uint8_t *)buf, buflen * 2);
      todo_len -= buflen;
    } else {
      ili9341_spi_write((uint8_t *)buf, todo_len * 2);
      todo_len = 0;
    }
  }
  free(buf);
}

static void ili9341_drawPixel(uint16_t x0, uint16_t y0) {
//  LOG(LL_DEBUG, ("Pixel [%d,%d] Window [%d,%d]-[%d,%d]", x0, y0, s_window.x0, s_window.y0, s_window.x1, s_window.y1));
  if (x0 + s_window.x0 > s_window.x1) {
    return;
  }
  if (y0 + s_window.y0 > s_window.y1) {
    return;
  }
  ili9341_set_clip(x0 + s_window.x0, y0 + s_window.y0, x0 + s_window.x0 + 1, y0 + s_window.y0 + 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  ili9341_spi_write((uint8_t *)&s_window.fg_color, 2);
}

// External primitives -- these are exported and all functions
// are declared non-static, start with mgos_ili9341_ and exposed
// in ili9341.h
// Helper functions are declared as static and named ili9341_*
uint16_t mgos_ili9341_color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

void mgos_ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  if (x0 > x1) {
    swap(x0, x1);
  }
  if (y0 > y1) {
    swap(y0, y1);
  }
  s_window.x0 = x0;
  s_window.x1 = x1;
  s_window.y0 = y0;
  s_window.y1 = y1;
}

void mgos_ili9341_set_fgcolor(uint8_t r, uint8_t g, uint8_t b) {
  s_window.fg_color = htons(mgos_ili9341_color565(r, g, b));
}

void mgos_ili9341_set_bgcolor(uint8_t r, uint8_t g, uint8_t b) {
  s_window.bg_color = htons(mgos_ili9341_color565(r, g, b));
}

void mgos_ili9341_set_fgcolor565(uint16_t rgb) {
  s_window.fg_color = htons(rgb);
}

void mgos_ili9341_set_bgcolor565(uint16_t rgb) {
  s_window.bg_color = htons(rgb);
}

void mgos_ili9341_set_dimensions(uint16_t width, uint16_t height) {
  s_screen_width  = width;
  s_screen_height = height;
}

/* Many screen implementations differ in orientation. Here's some application hints:
 * #define ADAFRUIT_LCD_PORTRAIT        (ILI9341_MADCTL_MX)
 * #define ADAFRUIT_LCD_LANDSCAPE       (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV)
 * #define ADAFRUIT_LCD_PORTRAIT_FLIP   (ILI9341_MADCTL_MY)
 * #define ADAFRUIT_LCD_LANDSCAPE_FLIP  (ILI9341_MADCTL_MV)
 *
 * #define M5STACK_LCD_PORTRAIT         (ILI9341_MADCTL_MV | ILI9341_MADCTL_MY)
 * #define M5STACK_LCD_LANDSCAPE        (ILI9341_MADCTL_NONE)
 * #define M5STACK_LCD_PORTRAIT_FLIP    (ILI9341_MADCTL_MV | ILI9341_MADCTL_MX)
 * #define M5STACK_LCD_LANDSCAPE_FLIP   (ILI9341_MADCTL_MY | ILI9341_MADCTL_ML | ILI9341_MADCTL_MX)
 */
void mgos_ili9341_set_orientation(uint8_t madctl, uint16_t rows, uint16_t cols) {
  madctl |= ILI9341_MADCTL_BGR;
  ili9341_spi_write8_cmd(ILI9341_MADCTL);
  ili9341_spi_write8(madctl);
  mgos_ili9341_set_dimensions(rows,cols);
  mgos_ili9341_set_window(0, 0, mgos_ili9341_get_screenWidth() - 1, mgos_ili9341_get_screenHeight() - 1);
}

void mgos_ili9341_set_rotation(enum mgos_ili9341_rotation_t rotation) {
  uint8_t madctl;
  uint16_t w=mgos_ili9341_get_screenWidth();
  uint16_t h=mgos_ili9341_get_screenHeight();

  switch (rotation) {
  case ILI9341_LANDSCAPE:
    madctl = (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
    if (w>h)
      mgos_ili9341_set_dimensions(w,h);
    else
      mgos_ili9341_set_dimensions(h,w);
    break;

  case ILI9341_LANDSCAPE_FLIP:
    madctl = (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
    if (w>h)
      mgos_ili9341_set_dimensions(w,h);
    else
      mgos_ili9341_set_dimensions(h,w);
    break;

  case ILI9341_PORTRAIT_FLIP:
    madctl = (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    if (h>w)
      mgos_ili9341_set_dimensions(w,h);
    else
      mgos_ili9341_set_dimensions(h,w);
    break;

  default:   // ILI9331_PORTRAIT
    madctl = (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
    if (h>w)
      mgos_ili9341_set_dimensions(w,h);
    else
      mgos_ili9341_set_dimensions(h,w);
  }
  ili9341_spi_write8_cmd(ILI9341_MADCTL);
  ili9341_spi_write8(madctl);
  mgos_ili9341_set_window(0, 0, mgos_ili9341_get_screenWidth() - 1, mgos_ili9341_get_screenHeight() - 1);
  return;
}

void mgos_ili9341_set_inverted(bool inverted) {
  if (inverted) {
    ili9341_spi_write8_cmd(ILI9341_INVON);
  } else{
    ili9341_spi_write8_cmd(ILI9341_INVOFF);
  }
}

#define swap(a, b)    { int16_t t = a; a = b; b = t; }
void mgos_ili9341_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  // Vertical line
  if (x0 == x1) {
    if (y1 < y0) {
      swap(y0, y1);
    }
    if (y0 + s_window.y0 > s_window.y1 || x0 + s_window.x0 > s_window.x1) {
//      LOG(LL_DEBUG, ("VLINE [%d,%d] length %d starts outside of window", x0, y0, y1));
      return;
    }
//    LOG(LL_DEBUG, ("VLINE [%d,%d]-[%d,%d] window [%d,%d]-[%d,%d]", x0, y0, x1, y1, s_window.x0, s_window.y0, s_window.x1, s_window.y1));
    if (y1 + s_window.y0 > s_window.y1) {
//      LOG(LL_DEBUG, ("VLINE [%d,%d] length %d ends outside of window, clipping it", x0, y0, y1-y0));
      y1 = s_window.y1 - s_window.y0;
    }
    return ili9341_fillRect(s_window.x0 + x0, s_window.y0 + y0, 1, y1 - y0);
  }

  // Horizontal line
  if (y0 == y1) {
    if (x1 < x0) {
      swap(x0, x1);
    }
    if (x0 + s_window.x0 > s_window.x1 || y0 + s_window.y0 > s_window.y1) {
//      LOG(LL_DEBUG, ("HLINE [%d,%d] length %d starts outside of window", x0, y0, y1));
      return;
    }
//    LOG(LL_DEBUG, ("HLINE [%d,%d]-[%d,%d] window [%d,%d]-[%d,%d]", x0, y0, x1, y1, s_window.x0, s_window.y0, s_window.x1, s_window.y1));
    if (x1 + s_window.x0 > s_window.x1) {
//      LOG(LL_DEBUG, ("HLINE [%d,%d] length %d ends outside of window, clipping it", x0, y0, x1-x0));
      x1 = s_window.x1 - s_window.x0;
    }
    return ili9341_fillRect(s_window.x0 + x0, s_window.y0 + y0, x1 - x0, 1);
  }

  int steep = 0;
  if (abs(y1 - y0) > abs(x1 - x0)) {
    steep = 1;
  }
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx = x1 - x0, dy = abs(y1 - y0);
  int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

  if (y0 < y1) {
    ystep = 1;
  }
  // Split into steep and not steep for FastH/V separation
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) {
          ili9341_drawPixel(y0, xs);
        } else{
          mgos_ili9341_drawLine(y0, xs, y0, xs + dlen);
        }
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) {
      mgos_ili9341_drawLine(y0, xs, y0, xs + dlen);
    }
  }else {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) {
          ili9341_drawPixel(xs, y0);
        } else{
          mgos_ili9341_drawLine(xs, y0, xs + dlen, y0);
        }
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) {
      mgos_ili9341_drawLine(xs, y0, xs + dlen, y0);
    }
  }
}

void mgos_ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  if (x0 + s_window.x0 > s_window.x1) {
    return;
  }
  if (y0 + s_window.y0 > s_window.y1) {
    return;
  }
  if (x0 + s_window.x0 + w > s_window.x1) {
    w = s_window.x1 - s_window.x0 - x0;
  }
  if (y0 + s_window.y0 + h > s_window.y1) {
    h = s_window.y1 - s_window.y0 - y0;
  }
  return ili9341_fillRect(s_window.x0 + x0, s_window.y0 + y0, w, h);
}

void mgos_ili9341_drawPixel(uint16_t x0, uint16_t y0) {
  return ili9341_drawPixel(x0, y0);
}

void mgos_ili9341_fillScreen() {
  return ili9341_fillRect(0, 0, s_screen_width, s_screen_height);
}

void mgos_ili9341_print(uint16_t x0, uint16_t y0, const char *string) {
  uint16_t  pixelline_width = 0;
  uint16_t *pixelline;
  uint16_t  lines;

  pixelline_width = mgos_ili9341_getStringWidth(string);
  if (pixelline_width == 0) {
    LOG(LL_ERROR, ("getStringWidth returned 0 -- is the font set?"));
    return;
  }

  lines = mgos_ili9341_getStringHeight(string);
  if (lines == 0) {
    LOG(LL_ERROR, ("getStringHeight returned 0 -- is the font set?"));
    return;
  }
  //LOG(LL_DEBUG, ("string='%s' at (%d,%d), width=%u height=%u", string, x0, y0, pixelline_width, lines));

  pixelline = calloc(pixelline_width, sizeof(uint16_t));
  if (!pixelline) {
    LOG(LL_ERROR, ("could not malloc for string='%s' at (%d,%d), width=%u height=%u", string, x0, y0, pixelline_width, lines));
    return;
  }

  for (int line = 0; line < mgos_ili9341_getStringHeight(string); line++) {
    int ret;
    for (int i = 0; i < pixelline_width; i++) {
      pixelline[i] = s_window.bg_color;
    }
    ret = ili9341_print_fillPixelLine(string, line, pixelline, s_window.fg_color);
    if (ret != pixelline_width) {
      LOG(LL_ERROR, ("ili9341_getStringPixelLine returned %d, but we expected %d", ret, pixelline_width));
    }
    ili9341_send_pixels(x0, y0 + line, x0 + pixelline_width - 1, y0 + line, (uint8_t *)pixelline, pixelline_width * sizeof(uint16_t));
  }
  free(pixelline);
}

void mgos_ili9341_printf(uint16_t x0, uint16_t y0, const char *fmt, ...) {
  char buf[50], *s = buf;
  va_list ap;
  va_start(ap, fmt);
  if (mg_avprintf(&s, sizeof(buf), fmt, ap) > 0) {
    mgos_ili9341_print(x0, y0, s);
  }
  va_end(ap);
  if (s != buf) free(s);
}

uint16_t mgos_ili9341_get_screenWidth() {
  return s_screen_width;
}

uint16_t mgos_ili9341_get_screenHeight() {
  return s_screen_height;
}

void mgos_ili9341_drawDIF(uint16_t x0, uint16_t y0, char *fn) {
  uint16_t *pixelline = NULL;
  uint8_t   dif_hdr[16];
  uint32_t  w, h;
  int       fd;

  fd = open(fn, O_RDONLY);
  if (!fd) {
    LOG(LL_ERROR, ("%s: Could not opens", fn));
    goto exit;
  }
  if (16 != read(fd, dif_hdr, 16)) {
    LOG(LL_ERROR, ("%s: Could not read DIF header", fn));
    goto exit;
  }
  if (dif_hdr[0] != 'D' || dif_hdr[1] != 'I' || dif_hdr[2] != 'F' || dif_hdr[3] != 1) {
    LOG(LL_ERROR, ("%s: Invalid DIF header", fn));
    goto exit;
  }
  w = dif_hdr[7] + (dif_hdr[6] << 8) + (dif_hdr[5] << 16) + (dif_hdr[4] << 24);
  h = dif_hdr[11] + (dif_hdr[10] << 8) + (dif_hdr[9] << 16) + (dif_hdr[8] << 24);
  LOG(LL_DEBUG, ("%s: width=%d height=%d", fn, w, h));
  pixelline = calloc(w, sizeof(uint16_t));

  for (uint16_t yy = 0; yy < h; yy++) {
    if (w * 2 != (uint16_t)read(fd, pixelline, w * 2)) {
      LOG(LL_ERROR, ("%s: short read", fn));
      goto exit;
    }
    ili9341_send_pixels(x0, y0 + yy, x0 + w - 1, y0 + yy, (uint8_t *)pixelline, w * sizeof(uint16_t));
    if (y0 + yy > s_window.y1) {
      break;
    }
  }

exit:
  if (pixelline) {
    free(pixelline);
  }
  close(fd);
}

bool mgos_ili9341_spi_init(void) {
  // Setup DC pin
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);
  mgos_gpio_set_mode(mgos_sys_config_get_ili9341_dc_pin(), MGOS_GPIO_MODE_OUTPUT);

  LOG(LL_INFO, ("ILI9341 init (CS%d, DC: %d, RST: %d, MODE: %d, FREQ: %d)",
                mgos_sys_config_get_ili9341_cs_index(),
                mgos_sys_config_get_ili9341_dc_pin(),
                mgos_sys_config_get_ili9341_rst_pin(),
                SPI_MODE, mgos_sys_config_get_ili9341_spi_freq()));

  if (mgos_sys_config_get_ili9341_rst_pin() >= 0) {
    // Issue a 20uS negative pulse on the reset pin and wait 5 mS so interface gets ready.
    mgos_gpio_write(mgos_sys_config_get_ili9341_rst_pin(), 1);
    mgos_gpio_set_mode(mgos_sys_config_get_ili9341_rst_pin(), MGOS_GPIO_MODE_OUTPUT);
    mgos_usleep(1000);
    mgos_gpio_write(mgos_sys_config_get_ili9341_rst_pin(), 0);
    mgos_usleep(20);
    mgos_gpio_write(mgos_sys_config_get_ili9341_rst_pin(), 1);
  }

  ili9341_commandList(ILI9341_init);

  mgos_ili9341_set_dimensions(mgos_sys_config_get_ili9341_width(), mgos_sys_config_get_ili9341_height());
  mgos_ili9341_set_rotation(ILI9341_LANDSCAPE);

  return true;
}
