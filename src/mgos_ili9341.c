#include "mgos_ili9341.h"
#include "mgos_ili9341_hal.h"
#include "mgos_ili9341_font.h"
#include "mgos_config.h"
#include "mgos_gpio.h"

#define SPI_MODE 0

struct ili9341_window {
  uint16_t x0;
  uint16_t x1;
  uint16_t y0;
  uint16_t y1;
  uint16_t fg_color; // in network byte order
  uint16_t bg_color; // in network byte order
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
      .freq = SPI_DEFAULT_FREQ,
  };
  txn.hd.tx_data = data,
  txn.hd.tx_len = size,
  txn.hd.dummy_len = 0,
  txn.hd.rx_len = 0,
  txn.hd.rx_data = NULL,
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
    mgos_ili9341_set_window(0, 0, 319, 239);
    madctl |= 1 << 5;
  } else {
    mgos_ili9341_set_dimensions(240, 320);
    mgos_ili9341_set_window(0, 0, 239, 319);
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

// buf represents a 16-bit RGB 565 uint16_t color buffer of length buflen bytes (so buflen/2 pixels).
// Note: data in 'buf' has to be in network byte order!
static void ili9341_send_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *buf, uint32_t buflen) {
  uint16_t winsize=(x1-x0+1)*(y1-y0+1);

  if (y0+s_window.y0>s_window.y1 || x0+s_window.x0>s_window.x1) {
    return;
  }
  if (y1+s_window.y0>s_window.y1) {
    y1=s_window.y1-s_window.y0;
  }
  if (x1+s_window.x0>s_window.x1) {
    x1=s_window.x1-s_window.x0;
  }

  if (buflen != winsize*2) {
    LOG(LL_ERROR, ("Want buflen(%d), to be twice the window size(%d)", buflen, winsize));
    return;
  }

  winsize=(x1-x0+1)*(y1-y0+1);

  ili9341_set_clip(x0+s_window.x0, y0+s_window.y0, x1+s_window.x0, y1+s_window.y0);
  ili9341_spi_write8_cmd(ILI9341_RAMWR);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  ili9341_spi_write(buf, winsize*2);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);
}

#define ILI9341_FILLRECT_CHUNK 256
static void ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  uint16_t *buf;
  uint32_t i;
  uint32_t todo_len;
  uint32_t buflen;

  todo_len=w*h;
  if (todo_len == 0)
    return;
  
  // Allocate at most 2*FILLRECT_CHUNK bytes
  buflen = (todo_len<ILI9341_FILLRECT_CHUNK?todo_len:ILI9341_FILLRECT_CHUNK);

  if (!(buf = calloc(buflen, sizeof(uint16_t))))
    return;

  for(i=0; i<buflen; i++) {
    buf[i] = s_window.fg_color;
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
//  LOG(LL_DEBUG, ("Pixel [%d,%d] Window [%d,%d]-[%d,%d]", x0, y0, s_window.x0, s_window.y0, s_window.x1, s_window.y1));
  if (x0+s_window.x0>s_window.x1) {
    return;
  }
  if (y0+s_window.y0>s_window.y1) {
    return;
  }
  ili9341_set_clip(x0+s_window.x0, y0+s_window.y0, x0+s_window.x0+1, y0+s_window.y0+1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 1);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 0);
  ili9341_spi_write((uint8_t *)&s_window.fg_color, 2);
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
  if (x0>x1) swap(x0, x1);
  if (y0>y1) swap(y0, y1);
  s_window.x0 = x0;
  s_window.x1 = x1;
  s_window.y0 = y0;
  s_window.y1 = y1;
}

void mgos_ili9341_set_fgcolor(uint8_t r, uint8_t g, uint8_t b) {
  s_window.fg_color=htons(mgos_ili9341_color565(r, g, b));
}

void mgos_ili9341_set_bgcolor(uint8_t r, uint8_t g, uint8_t b) {
  s_window.bg_color=htons(mgos_ili9341_color565(r, g, b));
}

void mgos_ili9341_set_fgcolor565(uint16_t rgb) {
  s_window.fg_color=htons(rgb);
}

void mgos_ili9341_set_bgcolor565(uint16_t rgb) {
  s_window.bg_color=htons(rgb);
}

void mgos_ili9341_set_dimensions(uint16_t width, uint16_t height) {
  s_screen_width = width;
  s_screen_height = height;
}

void mgos_ili9341_set_orientation(uint8_t flags) {
  return ili9341_set_orientation(flags);
}

void mgos_ili9341_set_inverted(bool inverted) {
  if (inverted)
    ili9341_spi_write8_cmd(ILI9341_INVON);
  else
    ili9341_spi_write8_cmd(ILI9341_INVOFF);
}


#define swap(a, b) { int16_t t = a; a = b; b = t; }
void mgos_ili9341_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  // Vertical line
  if (x0==x1) {
    if (y1<y0) swap(y0, y1);
    if (y0+s_window.y0>s_window.y1 || x0+s_window.x0>s_window.x1) {
//      LOG(LL_DEBUG, ("VLINE [%d,%d] length %d starts outside of window", x0, y0, y1));
      return;
    }
//    LOG(LL_DEBUG, ("VLINE [%d,%d]-[%d,%d] window [%d,%d]-[%d,%d]", x0, y0, x1, y1, s_window.x0, s_window.y0, s_window.x1, s_window.y1));
    if (y1+s_window.y0>s_window.y1) {
//      LOG(LL_DEBUG, ("VLINE [%d,%d] length %d ends outside of window, clipping it", x0, y0, y1-y0));
      y1=s_window.y1-s_window.y0;
    }
    return ili9341_fillRect(s_window.x0+x0, s_window.y0+y0, 1, y1-y0);
  }

  // Horizontal line
  if (y0==y1) {
    if (x1<x0) swap(x0, x1);
    if (x0+s_window.x0>s_window.x1 || y0+s_window.y0>s_window.y1) {
//      LOG(LL_DEBUG, ("HLINE [%d,%d] length %d starts outside of window", x0, y0, y1));
      return;
    }
//    LOG(LL_DEBUG, ("HLINE [%d,%d]-[%d,%d] window [%d,%d]-[%d,%d]", x0, y0, x1, y1, s_window.x0, s_window.y0, s_window.x1, s_window.y1));
    if (x1+s_window.x0>s_window.x1) {
//      LOG(LL_DEBUG, ("HLINE [%d,%d] length %d ends outside of window, clipping it", x0, y0, x1-x0));
      x1=s_window.x1-s_window.x0;
    }
    return ili9341_fillRect(s_window.x0+x0, s_window.y0+y0, x1-x0, 1);
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

void mgos_ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h) {
  if (x0+s_window.x0>s_window.x1)
    return;
  if (y0+s_window.y0>s_window.y1)
    return;
  if (x0+s_window.x0+w>s_window.x1)
    w=s_window.x1-s_window.x0-x0;
  if (y0+s_window.y0+h>s_window.y1)
    h=s_window.y1-s_window.y0-y0;
  return ili9341_fillRect(s_window.x0+x0, s_window.y0+y0, w, h);
}

void mgos_ili9341_drawPixel(uint16_t x0, uint16_t y0) {
  return ili9341_drawPixel(x0, y0);
}

void mgos_ili9341_fillScreen() {
  return ili9341_fillRect(0, 0, s_screen_width, s_screen_height);
}

void mgos_ili9341_print(uint16_t x0, uint16_t y0, char *string) {
  uint16_t pixelline_width=0;
  uint16_t *pixelline;
  uint16_t lines;

  pixelline_width = mgos_ili9341_getStringWidth(string);
  if (pixelline_width==0) {
    LOG(LL_ERROR, ("getStringWidth returned 0 -- is the font set?"));
    return;
  }

  lines = mgos_ili9341_getStringHeight(string);
  if (lines==0) {
    LOG(LL_ERROR, ("getStringHeight returned 0 -- is the font set?"));
    return;
  }
  LOG(LL_DEBUG, ("string='%s' at (%d,%d), width=%u height=%u", string, x0, y0, pixelline_width, lines));

  pixelline = calloc(pixelline_width, sizeof(uint16_t));
  if (!pixelline) {
    LOG(LL_ERROR, ("could not malloc for string='%s' at (%d,%d), width=%u height=%u", string, x0, y0, pixelline_width, lines));
    return;
  }

  for (int line=0; line<mgos_ili9341_getStringHeight(string); line++) {
    int ret;
    for (int i=0; i<pixelline_width; i++)
      pixelline[i]=s_window.bg_color;
    ret = ili9341_print_fillPixelLine(string, line, pixelline, s_window.fg_color);
    if (ret != pixelline_width)
      LOG(LL_ERROR, ("ili9341_getStringPixelLine returned %d, but we expected %d", ret, pixelline_width));
      ili9341_send_pixels(x0, y0+line, x0+pixelline_width-1, y0+line, (uint8_t *)pixelline, pixelline_width*sizeof(uint16_t));
  }
  free(pixelline);
}

uint16_t mgos_ili9341_get_screenWidth() {
  return s_screen_width;
}

uint16_t mgos_ili9341_get_screenHeight() {
  return s_screen_height;
}

void mgos_ili9341_drawDIF(uint16_t x0, uint16_t y0, char *fn) {
  uint16_t *pixelline = NULL;
  uint8_t dif_hdr[16];
  uint32_t w, h;
  int fd;
  
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
  w=dif_hdr[7] + (dif_hdr[6]<<8) + (dif_hdr[5]<<16) + (dif_hdr[4]<<24);
  h=dif_hdr[11] + (dif_hdr[10]<<8) + (dif_hdr[9]<<16) + (dif_hdr[8]<<24);
  LOG(LL_DEBUG, ("%s: width=%d height=%d", fn, w, h));
  pixelline = calloc(w, sizeof(uint16_t));

  for(int yy=0; yy<h; yy++) {
    if (w*2 != read(fd, pixelline, w*2)) {
      LOG(LL_ERROR, ("%s: short read", fn));
      goto exit;
    }
    ili9341_send_pixels(x0, y0+yy, x0+w-1, y0+yy, (uint8_t *)pixelline, w*sizeof(uint16_t));
    if (y0+yy>s_window.y1)
      break;
  }
  
exit:
  if (pixelline) free(pixelline);
  close(fd);
}

bool mgos_ili9341_spi_init(void) {
  // Setup CS pin
  mgos_gpio_set_mode(mgos_sys_config_get_ili9341_cs_pin(), MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(mgos_sys_config_get_ili9341_cs_pin(), 1);

  // Setup DC pin
  mgos_gpio_set_mode(mgos_sys_config_get_ili9341_dc_pin(), MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(mgos_sys_config_get_ili9341_dc_pin(), 0);

  LOG(LL_INFO, ("init (CS: %d, DC: %d, MODE: %d, FREQ: %d)", mgos_sys_config_get_ili9341_cs_pin(), mgos_sys_config_get_ili9341_dc_pin(), SPI_MODE, SPI_DEFAULT_FREQ));
  ili9341_commandList(ILI9341_init); 

  mgos_ili9341_set_orientation(ILI9341_SWITCH_XY | ILI9341_FLIP_X);

  ili9341_spi_write8_cmd(ILI9341_DISPON); //Display on

  return true;
}
