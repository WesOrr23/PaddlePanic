/*============================================================================
 * sh1106_graphics.c - Modified by Wes Orr (11/28/25)
 *============================================================================
 * Graphics library for SH1106-driven 128x64 B&W OLED displays via ATtiny1627
 * 
 * Based on Adafruit_GFX and Adafruit_GrayOLED libraries
 * Original Copyright (c) 2013 Adafruit Industries - BSD License
 *==========================================================================*/

#include <xc.h>
#include "ACU_SH1106_gfxlib.h"
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

/*============================================================================
 * MACROS
 *==========================================================================*/
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

/*============================================================================
 * DISPLAY BUFFER
 *==========================================================================*/
// Frame buffer: 128 columns × 8 pages (64 pixels / 8 bits per page)
uint8_t buffer[WIDTH * ((HEIGHT + 7) / 8)] = {0};

/*============================================================================
 * SPI INITIALIZATION
 *==========================================================================*/
void initSPI() {
    // Route SPI to alternate pins (PORTMUX)
    PORTMUX.SPIROUTEA |= PORTMUX_SPI0_ALT1_gc;
    
    // Configure SPI pins as outputs: PC0=SCLK, PC2=MOSI, PC3=SS
    PORTC.DIR |= PIN0_bm | PIN2_bm | PIN3_bm;
    
    // Configure SPI: Host mode, CLK/8 (CLK2X + DIV16/4), Mode 3
    SPI0.CTRLA |= SPI_MASTER_bm | SPI_CLK2X_bm | SPI_PRESC_DIV16_gc;
    SPI0.CTRLB |= SPI_MODE_3_gc;
    SPI0.CTRLA |= SPI_ENABLE_bm;
    
    PORTC.OUTSET = PIN3_bm;  // CS idle high
}

/*============================================================================
 * SPI COMMUNICATION
 *==========================================================================*/
void sendByteSPI(uint8_t byteToSend) {
    SPI0.DATA = byteToSend;
    PORTC.OUT &= ~PIN3_bm;  // CS low
    while ((SPI0.INTFLAGS & SPI_IF_bm) == 0) {}
    PORTC.OUT |= PIN3_bm;   // CS high
}

void sendCommand(uint8_t commByte) {
    PORTB.OUT &= ~0x02;  // D/C low for command
    sendByteSPI(commByte);
}

void sendData(uint8_t dataByte) {
    PORTB.OUT |= 0x02;   // D/C high for data
    sendByteSPI(dataByte);
}

/*============================================================================
 * DISPLAY INITIALIZATION
 *==========================================================================*/
void initScreen() {
    // Configure control pins: PB0=RES (reset), PB1=D/C (data/command)
    PORTB.DIR |= PIN0_bm | PIN1_bm;
    
    // Reset sequence: HIGH → LOW (>10µs) → HIGH
    PORTB.OUT |= PIN0_bm;
    for (uint8_t i = 0; i < 250; i++) {}
    PORTB.OUT &= ~PIN0_bm;
    for (uint16_t i = 0; i < 1000; i++) {}
    PORTB.OUT |= PIN0_bm;
    
    // SH1106 initialization sequence
    uint8_t initSequence[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide ratio
        0xA8, 0x3F, // Multiplex ratio (64)
        0xD3, 0x00, // Display offset
        0x40,       // Start line address
        0xAD, 0x8B, // DC-DC enable
        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins config
        0x81, 0xFF, // Contrast
        0xD9, 0x1F, // Pre-charge period
        0xDB, 0x40, // VCOM deselect level
        0x33,       // VPP
        0xA6,       // Normal display (not inverted)
        0x20, 0x00, // Memory addressing mode
        0x10,       // Column address high nibble
        0xA4        // Display resume (show RAM content)
    };
    
    for (uint8_t i = 0; i < sizeof(initSequence); i++) {
        sendCommand(initSequence[i]);
    }
    
    for (uint16_t i = 0; i < 40000; i++) {}  // Settle time
    
    sendCommand(0xAF);  // Display ON
}

/*============================================================================
 * PIXEL OPERATIONS
 *==========================================================================*/
/**
 * Set a pixel in the display buffer
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param color COLOR_WHITE, COLOR_BLACK, or COLOR_INVERT
 */
void writePixel(uint8_t x, uint8_t y, OLED_color color) {
    if ((x < WIDTH) && (y < HEIGHT)) {
        switch (color) {
            case COLOR_WHITE:
                buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
                break;
            case COLOR_BLACK:
                buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
                break;
            case COLOR_INVERT:
                buffer[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
                break;
        }
    }
}

/**
 * Get pixel state from display buffer
 * @return Non-zero if pixel is set, 0 if clear or out of bounds
 */
uint8_t getPixel(int16_t x, int16_t y) {
    if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
        return (buffer[x + (y / 8) * WIDTH] & (1 << (y & 7)));
    }
    return 0;
}

/*============================================================================
 * LINE DRAWING
 *==========================================================================*/
/**
 * Draw a line using Bresenham's algorithm
 */
void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_color color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }
    
    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            writePixel(y0, x0, color);
        } else {
            writePixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, OLED_color color) {
    writeLine(x, y, x, y + h - 1, color);
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, OLED_color color) {
    writeLine(x, y, x + w - 1, y, color);
}

/*============================================================================
 * RECTANGLE DRAWING
 *==========================================================================*/
/**
 * Draw a rectangle outline
 * @param x1,y1 Top-left corner
 * @param x2,y2 Bottom-right corner
 */
void writeRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_color color) {
    writeLine(x1, y1, x2, y1, color);
    writeLine(x2, y1, x2, y2, color);
    writeLine(x2, y2, x1, y2, color);
    writeLine(x1, y2, x1, y1, color);
}

/**
 * Draw a filled rectangle
 * @param x1,y1 Top-left corner
 * @param x2,y2 Bottom-right corner
 */
void writeFilledRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_color color) {
    // Ensure x1,y1 is top-left
    if (x2 < x1) {
        uint8_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y2 < y1) {
        uint8_t temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    for (; x1 <= x2; x1++) {
        writeLine(x1, y1, x1, y2, color);
    }
}

/*============================================================================
 * CIRCLE DRAWING
 *==========================================================================*/
/**
 * Draw a circle outline using midpoint circle algorithm
 * @param x0,y0 Center coordinates
 * @param r Radius
 */
void writeCircle(int16_t x0, int16_t y0, int16_t r, OLED_color color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    writePixel(x0, y0 + r, color);
    writePixel(x0, y0 - r, color);
    writePixel(x0 + r, y0, color);
    writePixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        writePixel(x0 + x, y0 + y, color);
        writePixel(x0 - x, y0 + y, color);
        writePixel(x0 + x, y0 - y, color);
        writePixel(x0 - x, y0 - y, color);
        writePixel(x0 + y, y0 + x, color);
        writePixel(x0 - y, y0 + x, color);
        writePixel(x0 + y, y0 - x, color);
        writePixel(x0 - y, y0 - x, color);
    }
}

/**
 * Draw a quarter circle (helper for rounded rectangles)
 * @param cornername Bitmap: bit 0=top-left, 1=top-right, 2=bottom-right, 3=bottom-left
 */
void writeQuarterCircle(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, OLED_color color) {
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
        
        if (cornername & 0x4) {
            writePixel(x0 + x, y0 + y, color);
            writePixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            writePixel(x0 + x, y0 - y, color);
            writePixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            writePixel(x0 - y, y0 + x, color);
            writePixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            writePixel(x0 - y, y0 - x, color);
            writePixel(x0 - x, y0 - y, color);
        }
    }
}

/**
 * Helper function for filled circles
 */
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, 
                      int16_t delta, OLED_color color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;
    
    delta++;
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if (x < (y + 1)) {
            if (corners & 1)
                drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2)
                drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1)
                drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
            if (corners & 2)
                drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

/**
 * Draw a filled circle
 */
void writeFilledCircle(int16_t x0, int16_t y0, int16_t r, OLED_color color) {
    drawFastVLine(x0, y0 - r, 2 * r + 1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

/*============================================================================
 * TRIANGLE DRAWING
 *==========================================================================*/
/**
 * Draw a triangle outline
 */
void writeTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                   int16_t x2, int16_t y2, OLED_color color) {
    writeLine(x0, y0, x1, y1, color);
    writeLine(x1, y1, x2, y2, color);
    writeLine(x2, y2, x0, y0, color);
}

/**
 * Draw a filled triangle
 */
void writeFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                         int16_t x2, int16_t y2, OLED_color color) {
    int16_t a, b, y, last;
    
    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1);
        _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1);
        _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1);
        _swap_int16_t(x0, x1);
    }
    
    // Handle degenerate case: all points on same line
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }
    
    int16_t dx01 = x1 - x0, dy01 = y1 - y0;
    int16_t dx02 = x2 - x0, dy02 = y2 - y0;
    int16_t dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    
    // Upper part of triangle
    last = (y1 == y2) ? y1 : y1 - 1;
    
    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) _swap_int16_t(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
    
    // Lower part of triangle
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) _swap_int16_t(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
}

/*============================================================================
 * BITMAP DRAWING
 *==========================================================================*/
/**
 * Draw a monochrome bitmap
 * @param x,y Top-left position
 * @param bitmap Pointer to bitmap data (1 bit per pixel, MSB first)
 * @param w,h Bitmap dimensions
 */
void writeBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, 
                 OLED_color color) {
    int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;
    
    for (int16_t j = 0; j < h; j++, y++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte = bitmap[j * byteWidth + i / 8];
            }
            if (byte & 0x80) {
                writePixel(x + i, y, color);
            }
        }
    }
}

/*============================================================================
 * DISPLAY CONTROL
 *==========================================================================*/
/**
 * Clear the display buffer (set all pixels to off)
 */
void clearDisplay(void) {
    memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
}

/**
 * Invert display colors (hardware-level)
 * @param inv 1 to invert, 0 for normal
 */
void invertDisplay(uint8_t inv) {
    if (inv) {
        sendCommand(GRAYOLED_INVERTDISPLAY);
    } else {
        sendCommand(GRAYOLED_NORMALDISPLAY);
    }
}

/**
 * Update the physical display with the contents of the buffer
 * Uses Read-Modify-Write mode for proper column addressing
 */
void showScreen() {
    // Write buffer to display in 8 pages (rows of 8-bit columns)
    for (uint8_t page = 0; page < 8; page++) {
        sendCommand(0xB0 | page);  // Set page address
        
        sendCommand(0xE0);  // Read-Modify-Write mode
        
        // Send 2 dummy bytes for column offset
        for (uint8_t i = 0; i < 2; i++) {
            sendData(0x00);
        }
        
        // Send 128 bytes of data for this page
        for (uint8_t col = 0; col < 128; col++) {
            sendData(buffer[(page * 128) + col]);
        }
        
        sendCommand(0xEE);  // End Read-Modify-Write
    }
}