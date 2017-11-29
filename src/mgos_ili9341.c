#include "mgos_ili9341.h"
#include "mgos_ili9341_hal.h"
#include "mgos_config.h"
#include "mgos_gpio.h"

#define SPI_SPEED 20000000
#define SPI_MODE 0

struct ili9341_window {
  uint16_t x0;
  uint16_t x1;
  uint16_t y0;
  uint16_t y1;
  uint16_t fg_color;
  uint16_t bg_color;
};

static uint16_t s_screen_width = 240;
static uint16_t s_screen_height = 320;
static struct ili9341_window s_window;

// SPI -- Hardware Interface, function names start with ili9341_spi_
// and are all declared static.
static void ili9341_spi_write(const uint8_t *data, uint32_t size) {
  struct mgos_spi *spi = mgos_spi_get_global();
  if (!spi) {
    LOG(LL_ERROR, ("Cannot get global SPI bus"));
    return;
  }

  struct mgos_spi_txn txn = {
      .cs = -1,
      .mode = SPI_MODE,
      .freq = SPI_SPEED,
      .hd.tx_data = data,
      .hd.tx_len = size,
  };
  mgos_spi_run_txn(spi, false, &txn);
}

static void ili9341_spi_write8_cmd(uint8_t byte) {
  // Command has DC low and CS low while writing to SPI bus.
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  ili9341_spi_write(&byte, 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
}

static void ili9341_spi_write8(uint8_t byte) {
  // Data has DC high and CS low while writing to SPI bus.
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  ili9341_spi_write(&byte, 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
}

// ILI9341 Primitives -- these methods call SPI commands directly,
// start with ili9341_ and are all declared static.
static void ili9341_commandList(const uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;                                // Number of commands to follow
  while(numCommands--) {                                // For each command...
    cmd = *addr++;                                      // save command
    numArgs  = *addr++;                                 // Number of args to follow
    ms       = numArgs & ILI9341_DELAY;                 // If high bit set, delay follows args
    numArgs &= ~ILI9341_DELAY;                          // Mask out delay bit

    mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);
    mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
    ili9341_spi_write(&cmd, 1);
    mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);

    mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
    mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
    ili9341_spi_write((uint8_t *)addr, numArgs);
    mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
    addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
      mgos_msleep(ms);
    }
  }
}

static void ili9341_set_orientation(uint8_t flags) {
  uint8_t madctl = 0x48;

  if (flags & ILI9341_FLIP_X)
    madctl &= ~(1 << 6);

  if (flags & ILI9341_FLIP_Y)
    madctl |= 1 << 7;

  if (flags & ILI9341_SWITCH_XY) {
    mgos_ili9341_set_dimensions(320, 240);
    madctl |= 1 << 5;
  } else {
    mgos_ili9341_set_dimensions(240, 320);
  }

  ili9341_spi_write8_cmd(ILI9341_MADCTL);
  ili9341_spi_write8(madctl);
}

static void ili9341_set_clip(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  ili9341_spi_write8_cmd(ILI9341_CASET); // Column addr set
  ili9341_spi_write8(x0 >> 8);
  ili9341_spi_write8(x0 & 0xFF); // XSTART
  ili9341_spi_write8(x1 >> 8);
  ili9341_spi_write8(x1 & 0xFF); // XEND
  ili9341_spi_write8_cmd(ILI9341_PASET); // Row addr set
  ili9341_spi_write8(y0>>8);
  ili9341_spi_write8(y0); // YSTART
  ili9341_spi_write8(y1>>8);
  ili9341_spi_write8(y1); // YEND
  ili9341_spi_write8_cmd(ILI9341_RAMWR); // write to RAM
  return;
}

// buf is a 16-bit RGB 565 color buffer of length len*2 bytes (so len pixels).
// Note: data in 'buf' has to be in network byte order!
/* TODO(pim): Implement later!
static void ili9341_send_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *buf, uint32_t len)
{
  ili9341_set_clip(x0, y0, x1, y1);

  ili9341_spi_write8_cmd(ILI9341_RAMWR);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  ili9341_spi_write(buf, len*2);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
}
*/

#define ILI9341_FILLRECT_CHUNK 256
static void ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h)
{
  uint16_t *buf;
  uint32_t i;
  uint32_t todo_len;
  uint32_t buflen;
  uint16_t color_htons;

  todo_len=w*h;
  if (todo_len == 0)
    return;
  
  // Allocate at most 2*FILLRECT_CHUNK bytes
  buflen = (todo_len<ILI9341_FILLRECT_CHUNK?todo_len:ILI9341_FILLRECT_CHUNK);

  if (!(buf = calloc(buflen, sizeof(uint16_t))))
    return;

  color_htons=htons(s_window.fg_color);
  for(i=0; i<buflen; i++) {
    buf[i] = color_htons;
  }
  ili9341_set_clip(x0, y0, x0+w-1, y0+h-1);
  ili9341_spi_write8_cmd(ILI9341_RAMWR);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  while (todo_len) {
    if (todo_len >= buflen) {
      ili9341_spi_write((uint8_t *)buf, buflen*2);
      todo_len -= buflen;
    } else {
      ili9341_spi_write((uint8_t *)buf, todo_len*2);
      todo_len=0;
    }
  }
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
  free(buf);
}

static void ili9341_drawPixel(uint16_t x0, uint16_t y0) {
  uint16_t pixel_color;

  pixel_color = htons(s_window.fg_color);
  ili9341_set_clip(x0, y0, x0+1, y0+1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  ili9341_spi_write((uint8_t *)&pixel_color, 2);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
}

// External primitives -- these are exported and all functions
// are declared non-static, start with mgos_ili9341_ and exposed
// in ili9341.h
// Helper functions are declared as static and named ili9341_*
uint16_t mgos_ili9341_color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

void mgos_ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  s_window.x0 = x0;
  s_window.x1 = x1;
  s_window.y0 = y0;
  s_window.y1 = y1;
}

void mgos_ili9341_set_fgcolor(uint8_t r, uint8_t g, uint8_t b) {
  s_window.fg_color=mgos_ili9341_color565(r, g, b);
}

void mgos_ili9341_set_bgcolor(uint8_t r, uint8_t g, uint8_t b) {
  s_window.bg_color=mgos_ili9341_color565(r, g, b);
}

void mgos_ili9341_set_fgcolor565(uint16_t rgb) {
  s_window.fg_color=rgb;
}

void mgos_ili9341_set_bgcolor565(uint16_t rgb) {
  s_window.bg_color=rgb;
}

void mgos_ili9341_set_dimensions(uint16_t width, uint16_t height) {
  s_screen_width = width;
  s_screen_height = height;
}

void mgos_ili9341_set_orientation(uint8_t flags) {
  return ili9341_set_orientation(flags);
}

#define swap(a, b) { int16_t t = a; a = b; b = t; }
void mgos_ili9341_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

  // Vertical line
  if (x0==x1) {
    if (y1>y0) return ili9341_fillRect(x0, y0, 1, y1-y0);
    else return ili9341_fillRect(x0, y1, 1, y0-y1);
  }

  // Horizontal line
  if (y0==y1) {
    if (x1>x0) return ili9341_fillRect(x0, y0, x1-x0, 1);
    else return ili9341_fillRect(x1, y0, x0-x1, 1);
  }

  int steep = 0;
  if (abs(y1 - y0) > abs(x1 - x0)) steep = 1;
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

  if (y0 < y1) ystep = 1;
  // Split into steep and not steep for FastH/V separation
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) ili9341_drawPixel(y0, xs);
        else mgos_ili9341_drawLine(y0, xs, y0, xs+dlen);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) mgos_ili9341_drawLine(y0, xs, y0, xs+dlen);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) ili9341_drawPixel(xs, y0);
        else mgos_ili9341_drawLine(xs, y0, xs+dlen, y0);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) mgos_ili9341_drawLine(xs, y0, xs+dlen, y0);
  }
}

static void ili9341_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername)
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
      ili9341_drawPixel(x0 - y, y0 - x);
      ili9341_drawPixel(x0 - x, y0 - y);
    }
    if (cornername & 0x2) {
      ili9341_drawPixel(x0 + x, y0 - y);
      ili9341_drawPixel(x0 + y, y0 - x);
    }
    if (cornername & 0x4) {
      ili9341_drawPixel(x0 + x, y0 + y);
      ili9341_drawPixel(x0 + y, y0 + x);
    }
    if (cornername & 0x8) {
      ili9341_drawPixel(x0 - y, y0 + x);
      ili9341_drawPixel(x0 - x, y0 + y);
    }
  }
}

static void ili9341_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t ylm = x0 - r;

  while (x < y) {
    if (f >= 0) {
      if (cornername & 0x1) mgos_ili9341_drawLine(x0+y, y0-x, x0+y, y0-x+2*x+1+delta);
      if (cornername & 0x2) mgos_ili9341_drawLine(x0-y, y0-x, x0-y, y0-x+2*x+1+delta);
      ylm = x0 - y;
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if ((x0 - x) > ylm) {
      if (cornername & 0x1) mgos_ili9341_drawLine(x0+x, y0-y, x0+x, y0-y+2*y+1+delta);
      if (cornername & 0x2) mgos_ili9341_drawLine(x0-x, y0-y, x0-x, y0-y+2*y+1+delta);
    }
  }
}

void mgos_ili9341_drawCircle(int16_t x, int16_t y, int radius) {
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x1 = 0;
  int y1 = radius;

  mgos_ili9341_drawPixel(x, y + radius);
  mgos_ili9341_drawPixel(x, y - radius);
  mgos_ili9341_drawPixel(x + radius, y);
  mgos_ili9341_drawPixel(x - radius, y);
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

void mgos_ili9341_fillCircle(int16_t x0, int16_t y0, int radius) {
  mgos_ili9341_drawLine(x0, y0-radius, x0, y0+radius+1);
  ili9341_fillCircleHelper(x0, y0, radius, 3, 0);
}


void mgos_ili9341_drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  mgos_ili9341_drawLine(x0, y0, x0+w-1, y0);
  mgos_ili9341_drawLine(x0+w-1, y0, x0+w-1, y0+h-1);
  mgos_ili9341_drawLine(x0, y0+h-1, x0+w-1, y0+h-1);
  mgos_ili9341_drawLine(x0, y0, x0, y0+h-1);
}

void mgos_ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  return ili9341_fillRect(x0, y0, w, h);
}

void mgos_ili9341_drawRoundRect(int16_t x0, int16_t y0, uint16_t w, uint16_t h, uint16_t r) {
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

void mgos_ili9341_fillRoundRect(int16_t x0, int16_t y0, uint16_t w, uint16_t h, uint16_t r) {
  ili9341_fillRect(x0+r, y0, w-2*r, h);
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


void mgos_ili9341_drawPixel(uint16_t x0, uint16_t y0) {
  return ili9341_drawPixel(x0, y0);
}

void mgos_ili9341_fillScreen() {
  return ili9341_fillRect(0, 0, s_screen_width, s_screen_height);
}

bool mgos_ili9341_spi_init(void) {
  // Setup CS pin
  mgos_gpio_set_mode(mgos_sys_config_get_ili9341_cs_pin(), MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);

  // Setup DC pin
  mgos_gpio_set_mode(mgos_sys_config_get_ili9341_dc_pin(), MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);

  LOG(LL_INFO, ("init (CS: %d, DC: %d, MODE: %d, FREQ: %d)", mgos_sys_config_get_ili9341_cs_pin(), mgos_sys_config_get_ili9341_dc_pin(), SPI_MODE, SPI_SPEED));
  ili9341_commandList(ILI9341_init); 

  ili9341_set_orientation(ILI9341_SWITCH_XY | ILI9341_FLIP_X);

  ili9341_spi_write8_cmd(ILI9341_DISPON); //Display on

  return true;
}
