## Attribution

These are files taken from Adafruit's GFX library:
https://github.com/adafruit/Adafruit-GFX-Library

## Modifications

*   `gfxfont.h`: added a few attributes to `struct GFXfont`
*   `fonts/*.h`: added the attributes from `struct GFXfont`
*   `fonts/*.h`: wrapped the data in `#ifndef X; #define X; #endif` stanzas
*   `src/mgos_ili9341_primitives.c`: Rewrote from C++ class to standard C
    implementation. Also, ensured each function takes (x,y) arguments that are
    unsigned ints, as this implementation sets a window to draw in.


