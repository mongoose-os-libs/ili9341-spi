let ILI9341 = {
    // Constants
    // Color definitions for RGB in 565-format
    BLACK: 0x0000, /*   0,   0,   0 */
    NAVY: 0x000F, /*   0,   0, 128 */
    DARKGREEN: 0x03E0, /*   0, 128,   0 */
    DARKCYAN: 0x03EF, /*   0, 128, 128 */
    MAROON: 0x7800, /* 128,   0,   0 */
    PURPLE: 0x780F, /* 128,   0, 128 */
    OLIVE: 0x7BE0, /* 128, 128,   0 */
    LIGHTGREY: 0xC618, /* 192, 192, 192 */
    DARKGREY: 0x7BEF, /* 128, 128, 128 */
    BLUE: 0x001F, /*   0,   0, 255 */
    GREEN: 0x07E0, /*   0, 255,   0 */
    CYAN: 0x07FF, /*   0, 255, 255 */
    RED: 0xF800, /* 255,   0,   0 */
    MAGENTA: 0xF81F, /* 255,   0, 255 */
    YELLOW: 0xFFE0, /* 255, 255,   0 */
    WHITE: 0xFFFF, /* 255, 255, 255 */
    ORANGE: 0xFD20, /* 255, 165,   0 */
    GREENYELLOW: 0xAFE5, /* 173, 255,  47 */
    PINK: 0xF81F,

    //enum mgos_ili9341_rotation_t
    PORTRAIT: 0,
    LANDSCAPE: 1,
    PORTRAIT_FLIP: 2,
    LANDSCAPE_FLIP: 3,

    // ffi functions
    // Externally callable functions:
    setWindow: ffi('void mgos_ili9341_set_window(int, int, int, int)'),
    setFgColor: ffi('void mgos_ili9341_set_fgcolor(int, int, int)'),
    setBgColor: ffi('void mgos_ili9341_set_bgcolor(int, int, int)'),
    color565: ffi('int mgos_ili9341_color565(int, int, int)'),
    setFgColor565: ffi('void mgos_ili9341_set_fgcolor565(int )'),
    setBgColor565: ffi('void mgos_ili9341_set_bgcolor565(int)'),
    setDimensions: ffi('void mgos_ili9341_set_dimensions(int, int)'),
    setOrientation: ffi('void mgos_ili9341_set_orientation(int, int, int)'),
    // argument is enum mgos_ili9341_rotation_t
    setRotation: ffi('void mgos_ili9341_set_rotation(int)'),
    setInverted: ffi('void mgos_ili9341_set_inverted(bool)'),
    getScreenWidth: ffi('int mgos_ili9341_get_screenWidth()'),
    getScreenHeight: ffi('int mgos_ili9341_get_screenHeight()'),

    fillScreen: ffi('void mgos_ili9341_fillScreen()'),

    // Geometric shapes:
    drawPixel: ffi('void mgos_ili9341_drawPixel(int, int)'),
    drawLine: ffi('void mgos_ili9341_drawLine(int, int, int, int)'),

    drawRect: ffi('void mgos_ili9341_drawRect(int, int, int, int)'),
    fillRect: ffi('void mgos_ili9341_fillRect(int, int, int, int)'),

    drawRoundRect: ffi('void mgos_ili9341_drawRoundRect(int, int, int, int, int)'),
    fillRoundRect: ffi('void mgos_ili9341_fillRoundRect(int, int, int, int, int)'),

    drawCircle: ffi('void mgos_ili9341_drawCircle(int, int, int)'),
    fillCircle: ffi('void mgos_ili9341_fillCircle(int, int, int)'),

    drawTriangle: ffi('void mgos_ili9341_drawTriangle(int, int, int, int, int, int)'),
    fillTriangle: ffi('void mgos_ili9341_fillTriangle(int, int, int, int, int, int)'),

    // Fonts and Printing:
    // argument is GFXfont*, need to find a way to get it
    setFont: ffi('bool mgos_ili9341_set_font(void*)'),
    print: ffi('void mgos_ili9341_print(int, int, char*)'),
    getStringWidth: ffi('int mgos_ili9341_getStringWidth(char*)'),
    getStringHeight: ffi('int mgos_ili9341_getStringHeight(char*)'),
    getMaxFontWidth: ffi('int mgos_ili9341_get_max_font_width()'),
    getMaxFontHeight: ffi('int mgos_ili9341_get_max_font_height()'),
    line: ffi('int mgos_ili9341_line(int)'),

    // Images
    drawDIF: ffi('void mgos_ili9341_drawDIF(int, int, char*)'),
};
