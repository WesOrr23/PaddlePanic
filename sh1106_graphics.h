/*============================================================================
 * sh1106_graphics.h - Modified by Wes Orr (11/28/25)
 *============================================================================
 * Graphics library for SH1106-driven 128x64 B&W OLED displays via ATtiny1627
 * 
 * Based on Adafruit_GFX and Adafruit_GrayOLED libraries
 * Original Copyright (c) 2013 Adafruit Industries - BSD License
 * Original version by Darby Hewitt (10/27/24)
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
 *     1. Draw to buffer using write... functions
 *     2. Call showScreen() to update physical display
 *     3. Call clearDisplay() to erase buffer (followed by showScreen())
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
void writePixel(Point pos, OLED_color color);

/**
 * Get pixel state from display buffer
 * @param pos Pixel coordinates
 * @return Non-zero if pixel is set, 0 if clear or out of bounds
 */
uint8_t getPixel(Point pos);

/*============================================================================
 * LINE DRAWING
 *==========================================================================*/
/**
 * Draw a line between two points using Bresenham's algorithm
 * @param start Starting point coordinates
 * @param end Ending point coordinates
 * @param color Line color
 */
void writeLine(Point start, Point end, OLED_color color);

/**
 * Draw a fast vertical line (optimized)
 * @param start Starting point (top of line)
 * @param height Line height in pixels
 * @param color Line color
 */
void drawFastVLine(Point start, int16_t height, OLED_color color);

/**
 * Draw a fast horizontal line (optimized)
 * @param start Starting point (left of line)
 * @param width Line width in pixels
 * @param color Line color
 */
void drawFastHLine(Point start, int16_t width, OLED_color color);

/*============================================================================
 * RECTANGLE DRAWING
 *==========================================================================*/
/**
 * Draw a rectangle outline
 * @param topLeft Top-left corner coordinates
 * @param bottomRight Bottom-right corner coordinates
 * @param color Rectangle color
 */
void writeRect(Point topLeft, Point bottomRight, OLED_color color);

/**
 * Draw a filled rectangle
 * @param topLeft Top-left corner coordinates
 * @param bottomRight Bottom-right corner coordinates
 * @param color Rectangle fill color
 */
void writeFilledRect(Point topLeft, Point bottomRight, OLED_color color);

/*============================================================================
 * CIRCLE DRAWING
 *==========================================================================*/
/**
 * Draw a circle outline using midpoint circle algorithm
 * @param center Center coordinates
 * @param radius Circle radius in pixels
 * @param color Circle color
 */
void writeCircle(Point center, int16_t radius, OLED_color color);

/**
 * Draw a filled circle
 * @param center Center coordinates
 * @param radius Circle radius in pixels
 * @param color Circle fill color
 */
void writeFilledCircle(Point center, int16_t radius, OLED_color color);

/**
 * Draw a quarter circle (helper for rounded rectangles)
 * @param center Center coordinates
 * @param radius Circle radius in pixels
 * @param cornerName Bitmap: bit 0=NW, 1=NE, 2=SE, 3=SW
 * @param color Circle color
 */
void writeQuarterCircle(Point center, int16_t radius, uint8_t cornerName, OLED_color color);

/**
 * Helper function for filled circles (internal use)
 */
void fillCircleHelper(Point center, int16_t radius, uint8_t corners, 
                      int16_t delta, OLED_color color);

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
void writeBitmap(Point pos, uint8_t *bitmap, int16_t width, int16_t height, 
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
void showScreen(void);

#endif // SH1106_GRAPHICS_H