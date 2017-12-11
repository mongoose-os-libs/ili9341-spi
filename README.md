# ILI9341 TFT Driver

## Introduction
This library is a native implementation of a graphics chip for small TFT
screens, the ILI9341. The chip is found in many popular displays, including
320x240 and 480x320 pixel versions, and is capable of driving 16-bit
(`RGB-565`) as well as 18-bit (`RGB-666`) modes. Although the chip is capable
of using 18-bit colors (262,114), that requires three bytes per pixel to be
written. Therefore, this driver initializes it in 16-bit colors (65,536)
allowing for two bytes per pixel.

## Fundamentals

The chip allows access to its framebuffer memory by setting a window by means
of (x0,y0)-(x1,y1) coordinates, and then writing pixel data. To implement
higher level shapes and fonts, we implement three basic functions on the
hardware:

```c
void mgos_ili9341_drawPixel(uint16_t x0, uint16_t y0);
```
This function sets the window to be exactly 1 pixel and writes the current
foreground color to it.

```c
void mgos_ili9341_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
```
This function can draw a vertical line by setting the window to
(x0,y0)-(x0+1,y1), and writing the current foreground color to it. It can draw
a horizontal line similarly by setting the window to (x0,y0)-(x1,y0+1). Any
other lines can be decomposed into segments of horizontal and vertical lines
and single pixels.

```c
void mgos_ili9341_fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
```
This sets the window to (x0,y0)-(x0+w-1,y0+h-1) and writes the current
foreground color to it.

## Primitives

### Window and Clipping

The driver works by setting a bounding box around the area to be drawn in:
```c
void mgos_ili9341_set_window(uint16_t x0, uint16_t y0,
                             uint16_t x1, uint16_t y1);
```

Any operation will be homed within the window, and clipped if it runs outside of
the window. For example, setting the window to (20,30)-(99,99) will create a
bounding box of 80 pixels wide and 70 pixels high. Subsequently calling
`mgos_ili9341_drawPixel(0,0)` will draw a pixel at the top left of the window,
which in this case is physical pixel (20,30). Drawing a pixel at coordinates
(80,70) will draw a pixel at the bottom right corner of the window, whereas
drawing one at (81,71) will not show anything.


### Geometric Shapes

A set of primitives are provided to allow drawing of geometrics shapes such as
circles, triangles and boxes:

```c
void mgos_ili9341_drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
void mgos_ili9341_drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w,
                                uint16_t h, uint16_t r);
void mgos_ili9341_fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w,
                                uint16_t h, uint16_t r);

void mgos_ili9341_drawCircle(uint16_t x0, uint16_t y0, uint16_t r);
void mgos_ili9341_fillCircle(uint16_t x0, uint16_t y0, uint16_t r);

void mgos_ili9341_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1,
                               uint16_t y1, uint16_t x2, uint16_t y2);
void mgos_ili9341_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1,
                               uint16_t y1, uint16_t x2, uint16_t y2);
```

### Fonts

Fonts can be embedded in the program by including the font files in `fonts/*.h`.
The width and height of a given string can be calculated, and a string can be
printed (using the set foreground and background colors). As with geometric
shapes, the print function is relative to the window, (0,0) being top-left, and
will be clipped at the window borders.

### Images

Since the ILI9341 chip expects pixels in 16-bit network byte order, we can
transmit a line of pixels from an image. Because the build target of the driver
is Mongoose OS's available platforms (notably ESP8266, ESP32 and CC32xx), memory
is an issue, and decompressing `PNG`, `GIF`, or `JPG` files can be CPU as well
as memory intensive, a simplistic alternative is provided: `DIF`

#### DIF file format

`DIF` stands for __dumb image format__, and consists of a 16 byte header
followed by raw pixel data, 16 bits per pixel, in network byte order. This
aleviates CPU and memory pressure on the micro controller, at the expense of
file size. The header is as follows:

*   The first four bytes (offset 0..3) are the string `DIF\001`
*   The second four bytes (offset 4..7) are the width (an `uint32_t` in network
    byte order).
*   The third four bytes (offset 8..11) are the height (an `uint32_t` in network
    byte order).
*   The byte at offset 12 denotes the raw image format that follows:
    *   Value of 0 means: 16 bit `RGB-565` in network byte order.
*   The last three bytes (offset 13..15) are reserved for future use.

The file size will therefor be ***`w` * `h` + 16*** bytes.

# Disclaimer

This project is not an official Google project. It is not supported by Google
and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.
