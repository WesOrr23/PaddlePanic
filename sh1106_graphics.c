/*============================================================================
 * sh1106_graphics.c - Modified by Wes Orr (11/29/25)
 *============================================================================
 * Low-level graphics primitives for SH1106-driven 128x64 B&W OLED displays
 * via ATtiny1627
 * 
 * Based on Adafruit_GFX and Adafruit_GrayOLED libraries
 * Original Copyright (c) 2013 Adafruit Industries - BSD License
 * Original version by Darby Hewitt (10/27/24)
 * 
 * This file provides low-level display primitives (pixels, lines, bitmaps).
 * All shape-specific code (circles, rectangles) is in shapes.c
 *==========================================================================*/

#include <xc.h>
#include "sh1106_graphics.h"
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

/*============================================================================
 * MACROS
 *==========================================================================*/
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/*============================================================================
 * DISPLAY BUFFER
 *==========================================================================*/
// Frame buffer organized as 8 pages of 128 bytes each
// Each byte represents 8 vertical pixels (bit 0 = top, bit 7 = bottom)
// Total: 128 columns × 64 rows = 1024 bytes
uint8_t buffer[WIDTH * ((HEIGHT + 7) / 8)] = {0};

/*============================================================================
 * SPI INITIALIZATION
 *==========================================================================*/
void initSPI() {
    PORTMUX.SPIROUTEA |= PORTMUX_SPI0_ALT1_gc;                               // Route SPI0 to alternate pin locations
    
    PORTC.DIR |= PIN0_bm | PIN2_bm | PIN3_bm;                                // Set as outputs: PC0=SCLK, PC2=MOSI, PC3=SS
    
    SPI0.CTRLA |= SPI_MASTER_bm | SPI_CLK2X_bm | SPI_PRESC_DIV16_gc;         // Host mode, double speed enabled, prescaler /16 → f_clk/8
    SPI0.CTRLB |= SPI_MODE_3_gc;                                             // CPOL=1, CPHA=1 (idle high, sample on rising edge)
    SPI0.CTRLA |= SPI_ENABLE_bm;                                             // Enable SPI peripheral
    
    PORTC.OUTSET = PIN3_bm;                                                  // Deassert CS (active low, so idle high)
}

/*============================================================================
 * SPI COMMUNICATION
 *==========================================================================*/
void sendByteSPI(uint8_t byteToSend) {
    SPI0.DATA = byteToSend;                                                  // Load byte into transmit buffer
    PORTC.OUT &= ~PIN3_bm;                                                   // Assert CS (active low)
    while ((SPI0.INTFLAGS & SPI_IF_bm) == 0) {}                              // Wait for transfer complete
    PORTC.OUT |= PIN3_bm;                                                    // Deassert CS
}

void sendCommand(uint8_t commandByte) {
    PORTB.OUT &= ~0x02;                                                      // Set D/C low (command mode)
    sendByteSPI(commandByte);
}

void sendData(uint8_t dataByte) {
    PORTB.OUT |= 0x02;                                                       // Set D/C high (data mode)
    sendByteSPI(dataByte);
}

/*============================================================================
 * DISPLAY INITIALIZATION
 *==========================================================================*/
void initScreen() {
    // Initialize SPI first (screen requires SPI)
    initSPI();

    PORTB.DIR |= PIN0_bm | PIN1_bm;                                          // Set as outputs: PB0=RESET, PB1=D/C

    // Hardware reset sequence per SH1106 datasheet
    PORTB.OUT |= PIN0_bm;                                                    // RESET high (inactive)
    for (uint8_t i = 0; i < 250; i++) {}                                     // Short delay
    PORTB.OUT &= ~PIN0_bm;                                                   // RESET low (active) for >10µs
    for (uint16_t i = 0; i < 1000; i++) {}                                   // Hold in reset
    PORTB.OUT |= PIN0_bm;                                                    // RESET high (release)
    
    // SH1106 initialization commands
    uint8_t initSequence[] = {
        0xAE,       // Display OFF (sleep mode)
        0xD5, 0x80, // Set display clock divide ratio (default)
        0xA8, 0x3F, // Set multiplex ratio to 64 (for 64-row display)
        0xD3, 0x00, // Set display offset to 0
        0x40,       // Set display start line to 0
        0xAD, 0x8B, // Enable internal DC-DC converter (for OLED power)
        0xA1,       // Set segment remap (flip horizontal)
        0xC8,       // Set COM output scan direction (flip vertical)
        0xDA, 0x12, // Set COM pins hardware configuration
        0x81, 0xFF, // Set contrast to maximum
        0xD9, 0x1F, // Set pre-charge period
        0xDB, 0x40, // Set VCOMH deselect level
        0x33,       // Set VPP to 9V
        0xA6,       // Set normal display mode (0=off, 1=on)
        0x20, 0x00, // Set memory addressing mode to horizontal
        0x10,       // Set higher column start address to 0
        0xA4        // Resume displaying from RAM content (not all-on)
    };
    
    for (uint8_t i = 0; i < sizeof(initSequence); i++) {
        sendCommand(initSequence[i]);
    }
    
    for (uint16_t i = 0; i < 40000; i++) {}                                  // Allow display to stabilize
    
    sendCommand(0xAF);                                                       // Display ON
}

/*============================================================================
 * PIXEL OPERATIONS
 *==========================================================================*/
/**
 * Set a pixel in the display buffer
 * @param pos Pixel coordinates (0-127, 0-63)
 * @param color COLOR_WHITE, COLOR_BLACK, or COLOR_INVERT
 */
void drawPixel(Point pos, OLED_color color) {
    if ((pos.x < WIDTH) && (pos.y < HEIGHT)) {
        // Calculate buffer position: column + (page * width)
        // Each page is 8 pixels tall, bit position determined by (y & 7)
        switch (color) {
            case COLOR_WHITE:
                buffer[pos.x + (pos.y / 8) * WIDTH] |= (1 << (pos.y & 7));   // Set bit
                break;
            case COLOR_BLACK:
                buffer[pos.x + (pos.y / 8) * WIDTH] &= ~(1 << (pos.y & 7));  // Clear bit
                break;
            case COLOR_INVERT:
                buffer[pos.x + (pos.y / 8) * WIDTH] ^= (1 << (pos.y & 7));   // Toggle bit
                break;
        }
    }
}

/**
 * Get pixel state from display buffer
 * @param pos Pixel coordinates
 * @return Non-zero if pixel is set, 0 if clear or out of bounds
 */
uint8_t getPixel(Point pos) {
    if ((pos.x >= 0) && (pos.x < WIDTH) && (pos.y >= 0) && (pos.y < HEIGHT)) {
        return (buffer[pos.x + (pos.y / 8) * WIDTH] & (1 << (pos.y & 7)));
    }
    return 0;                                                                // Out of bounds returns "off"
}

/*============================================================================
 * LINE DRAWING WITH CLIPPING
 *==========================================================================*/

/**
 * Cohen-Sutherland line clipping outcodes
 */
#define OUTCODE_INSIDE 0  // 0000
#define OUTCODE_LEFT   1  // 0001
#define OUTCODE_RIGHT  2  // 0010
#define OUTCODE_BOTTOM 4  // 0100
#define OUTCODE_TOP    8  // 1000

/**
 * Compute outcode for point relative to screen bounds
 */
static uint8_t compute_outcode(int16_t x, int16_t y) {
    uint8_t code = OUTCODE_INSIDE;
    
    if (x < 0) code |= OUTCODE_LEFT;
    else if (x >= WIDTH) code |= OUTCODE_RIGHT;
    
    if (y < 0) code |= OUTCODE_TOP;
    else if (y >= HEIGHT) code |= OUTCODE_BOTTOM;
    
    return code;
}

/**
 * Clip line to screen bounds using Cohen-Sutherland algorithm
 * Returns 1 if line is visible (even partially), 0 if completely outside
 */
static uint8_t clip_line(int16_t* x0, int16_t* y0, int16_t* x1, int16_t* y1) {
    uint8_t outcode0 = compute_outcode(*x0, *y0);
    uint8_t outcode1 = compute_outcode(*x1, *y1);
    
    while (1) {
        if (!(outcode0 | outcode1)) {
            // Both points inside - accept
            return 1;
        } else if (outcode0 & outcode1) {
            // Both points share an outside region - reject
            return 0;
        } else {
            // Line crosses boundary - clip it
            int16_t x, y;
            uint8_t outcode_out = outcode0 ? outcode0 : outcode1;
            
            // Find intersection point
            if (outcode_out & OUTCODE_BOTTOM) {
                x = *x0 + (*x1 - *x0) * (HEIGHT - 1 - *y0) / (*y1 - *y0);
                y = HEIGHT - 1;
            } else if (outcode_out & OUTCODE_TOP) {
                x = *x0 + (*x1 - *x0) * (0 - *y0) / (*y1 - *y0);
                y = 0;
            } else if (outcode_out & OUTCODE_RIGHT) {
                y = *y0 + (*y1 - *y0) * (WIDTH - 1 - *x0) / (*x1 - *x0);
                x = WIDTH - 1;
            } else { // OUTCODE_LEFT
                y = *y0 + (*y1 - *y0) * (0 - *x0) / (*x1 - *x0);
                x = 0;
            }
            
            // Replace outside point with intersection point
            if (outcode_out == outcode0) {
                *x0 = x;
                *y0 = y;
                outcode0 = compute_outcode(*x0, *y0);
            } else {
                *x1 = x;
                *y1 = y;
                outcode1 = compute_outcode(*x1, *y1);
            }
        }
    }
}

/**
 * Draw a line using Bresenham's algorithm with clipping
 * Lines that extend off-screen are clipped to screen boundaries
 */
void drawLine(Point start, Point end, OLED_color color) {
    int16_t x0 = start.x, y0 = start.y;
    int16_t x1 = end.x, y1 = end.y;
    
    // Clip line to screen bounds
    if (!clip_line(&x0, &y0, &x1, &y1)) {
        return;  // Line is completely outside screen
    }
    
    // Check if line is steep (more vertical than horizontal)
    int16_t isSteep = abs(y1 - y0) > abs(x1 - x0);
    
    // For steep lines, swap x and y coordinates to use same algorithm
    if (isSteep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }
    
    // Ensure we're always drawing left to right
    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }
    
    int16_t deltaX = x1 - x0;                                                // Horizontal distance
    int16_t deltaY = abs(y1 - y0);                                           // Vertical distance (absolute)
    int16_t error = deltaX / 2;                                              // Accumulated error for decision
    int16_t yStep = (y0 < y1) ? 1 : -1;                                      // Direction to step in y
    
    // Draw line pixel by pixel
    for (; x0 <= x1; x0++) {
        if (isSteep) {
            drawPixel((Point){y0, x0}, color);                              // Swap back for steep lines
        } else {
            drawPixel((Point){x0, y0}, color);
        }
        error -= deltaY;
        if (error < 0) {                                                     // Time to step in y direction
            y0 += yStep;
            error += deltaX;
        }
    }
}

/*============================================================================
 * BITMAP DRAWING
 *==========================================================================*/
/**
 * Draw a monochrome bitmap
 * Bitmap format: 1 bit per pixel, packed into bytes, MSB first
 * @param pos Top-left position
 * @param bitmap Pointer to bitmap data in memory
 * @param width Bitmap width in pixels
 * @param height Bitmap height in pixels
 */
void drawBitmap(Point pos, uint8_t *bitmap, int16_t width, int16_t height, 
                 OLED_color color) {
    int16_t byteWidth = (width + 7) / 8;                                     // Bytes per row (rounded up)
    uint8_t currentByte = 0;
    
    for (int16_t row = 0; row < height; row++, pos.y++) {
        for (int16_t col = 0; col < width; col++) {
            if (col & 7) {                                                   // Not on byte boundary
                currentByte <<= 1;                                           // Shift to next bit
            } else {                                                         // Start of new byte
                currentByte = bitmap[row * byteWidth + col / 8];
            }
            if (currentByte & 0x80) {                                        // Check MSB (current pixel)
                drawPixel((Point){pos.x + col, pos.y}, color);
            }
        }
    }
}

/*============================================================================
 * DISPLAY CONTROL
 *==========================================================================*/
/**
 * Clear the display buffer (set all pixels to off)
 * Does not update display - call showScreen() to make visible
 */
void clearDisplay(void) {
    memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
}

/**
 * Invert display colors at hardware level
 * This is faster than redrawing and affects entire display
 * @param invert 1 to invert all pixels, 0 for normal display
 */
void invertDisplay(uint8_t invert) {
    if (invert) {
        sendCommand(GRAYOLED_INVERTDISPLAY);
    } else {
        sendCommand(GRAYOLED_NORMALDISPLAY);
    }
}

/**
 * Update the physical display with contents of buffer
 * SH1106 requires Read-Modify-Write mode due to 132-column controller
 * driving 128-column display (2-pixel offset)
 */
void refreshDisplay() {
    for (uint8_t page = 0; page < 8; page++) {
        sendCommand(0xB0 | page);                                            // Set page address (0-7)
        
        sendCommand(0xE0);                                                   // Enter Read-Modify-Write mode
        
        // Send 2 dummy bytes to account for 2-column offset of SH1106
        for (uint8_t i = 0; i < 2; i++) {
            sendData(0x00);
        }
        
        // Send all 128 columns of data for this page
        for (uint8_t col = 0; col < 128; col++) {
            sendData(buffer[(page * 128) + col]);
        }
        
        sendCommand(0xEE);                                                   // Exit Read-Modify-Write mode
    }
}