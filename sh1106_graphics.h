/*============================================================================
 * sh1106_graphics.h - Modified by Wes Orr (11/29/25)
 *============================================================================
 * Low-level graphics primitives for SH1106-driven 128x64 B&W OLED displays
 * via ATtiny1627
 * 
 * Based on Adafruit_GFX and Adafruit_GrayOLED libraries
 * Original Copyright (c) 2013 Adafruit Industries - BSD License
 * Original version by Darby Hewitt (10/27/24)
 *
 * This library provides low-level display primitives (pixels, lines, bitmaps).
 * For shape drawing (circles, rectangles), use shapes.h which provides an
 * object-oriented interface built on top of these primitives.
 *
 *============================================================================
 * HARDWARE SETUP
 *============================================================================
 * Using ATtiny1627 (e.g., Curiosity Nano) with SH1106 128x64 OLED display:
 *
 *     Display Pin    <->    ATtiny1627 Pin
 *     -----------          ---------------
 *     CLK            <->    PC0 (SPI Clock)
 *     MOSI           <->    PC2 (SPI Data Out)
 *     RES            <->    PB0 (Reset)
 *     DC             <->    PB1 (Data/Command)
 *     CS             <->    PC3 (Chip Select)
 *
 * INITIALIZATION
 *     In main(), call in order:
 *     1. initSPI()
 *     2. initScreen()
 *
 * USAGE
 *     Low-level approach (pixels, lines, bitmaps):
 *         writePixel((Point){10, 10}, COLOR_WHITE);
 *         writeLine((Point){0, 0}, (Point){127, 63}, COLOR_WHITE);
 *         showScreen();
 *     
 *     Object-oriented approach (recommended for shapes):
 *         #include "shapes.h"
 *         Shape* circle = create_circle((Point){64, 32}, 15, 1);
 *         shape_draw(circle, COLOR_WHITE);
 *         showScreen();
 *
 *==========================================================================*/

#ifndef SH1106_GRAPHICS_H
#define SH1106_GRAPHICS_H

#include <stdint.h>

/*============================================================================
 * DISPLAY PARAMETERS
 *==========================================================================*/
#define WIDTH   128                                                          // Display width in pixels
#define HEIGHT  64                                                           // Display height in pixels

/*============================================================================
 * DISPLAY COMMANDS
 *==========================================================================*/
#define GRAYOLED_NORMALDISPLAY 0xA6                                          // Normal display mode (0=off, 1=on)
#define GRAYOLED_INVERTDISPLAY 0xA7                                          // Inverted display mode (0=on, 1=off)

/*============================================================================
 * TYPE DEFINITIONS
 *==========================================================================*/
/**
 * Color enumeration for monochrome display
 * COLOR_BLACK  - Pixel off
 * COLOR_WHITE  - Pixel on
 * COLOR_INVERT - Toggle pixel state
 */
typedef enum {
    COLOR_BLACK,
    COLOR_WHITE,
    COLOR_INVERT
} OLED_color;

/**
 * Point structure for coordinate pairs
 * Used throughout API for cleaner function signatures
 */
typedef struct {
    uint8_t x;
    uint8_t y;
} Point;

/*============================================================================
 * INITIALIZATION FUNCTIONS
 *==========================================================================*/
/**
 * Initialize SPI peripheral
 * Configures SPI0 in host mode, Mode 3, at f_clk/8
 * Must be called before initScreen()
 */
void initSPI(void);

/**
 * Initialize the OLED display
 * Performs hardware reset and sends initialization sequence
 * Must be called after initSPI()
 */
void initScreen(void);

/*============================================================================
 * SPI COMMUNICATION (Internal use)
 *==========================================================================*/
void sendByteSPI(uint8_t byteToSend);
void sendCommand(uint8_t commandByte);
void sendData(uint8_t dataByte);

/*============================================================================
 * PIXEL OPERATIONS
 *==========================================================================*/
/**
 * Set a pixel in the display buffer
 * @param pos Pixel coordinates (0-127, 0-63)
 * @param color COLOR_WHITE, COLOR_BLACK, or COLOR_INVERT
 */
void drawPixel(Point pos, OLED_color color);

/**
 * Get pixel state from display buffer
 * @param pos Pixel coordinates
 * @return Non-zero if pixel is set, 0 if clear or out of bounds
 */
uint8_t getPixel(Point pos);

/*============================================================================
 * LINE DRAWING PRIMITIVES
 *==========================================================================*/
/**
 * Draw a line between two points using Bresenham's algorithm
 * This is a fundamental primitive used by shape drawing functions
 * @param start Starting point coordinates
 * @param end Ending point coordinates
 * @param color Line color
 */
void drawLine(Point start, Point end, OLED_color color);

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
 * @param color Color to draw set pixels (1 bits)
 */
void drawBitmap(Point pos, uint8_t *bitmap, int16_t width, int16_t height, 
                 OLED_color color);

/*============================================================================
 * DISPLAY CONTROL
 *==========================================================================*/
/**
 * Clear the display buffer (set all pixels to off)
 * Does not update physical display - call showScreen() to make visible
 */
void clearDisplay(void);

/**
 * Invert display colors at hardware level
 * @param invert 1 to invert all pixels, 0 for normal display
 */
void invertDisplay(uint8_t invert);

/**
 * Update the physical display with contents of buffer
 * Call this after drawing operations to make changes visible
 */
void refreshDisplay(void);

#endif // SH1106_GRAPHICS_H