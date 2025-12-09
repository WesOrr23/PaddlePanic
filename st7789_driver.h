/*============================================================================
 * st7789_driver.h - Created by Wes Orr (12/8/25)
 *============================================================================
 * Minimal ST7789 240x240 RGB display driver
 * Based on Adafruit implementation, adapted for ATtiny1627
 *==========================================================================*/

#ifndef ST7789_DRIVER_H
#define ST7789_DRIVER_H

#include <stdint.h>

/*============================================================================
 * DISPLAY DIMENSIONS
 *==========================================================================*/
#define ST7789_WIDTH  240
#define ST7789_HEIGHT 240

// Column/row offsets (some 240x240 displays need this)
// Try (0,0) first, then (0,80) if display is shifted
#define ST7789_XSTART 0
#define ST7789_YSTART 0

/*============================================================================
 * COLOR TYPE (RGB565)
 *==========================================================================*/
typedef uint16_t ST7789_Color;

/*============================================================================
 * COLOR UTILITIES
 *==========================================================================*/

/**
 * Convert grayscale (0-255) to RGB565
 * For grayscale: R=G=B
 * @param gray Grayscale value 0-255 (0=black, 255=white)
 * @return RGB565 color value
 */
ST7789_Color st7789_grayscale(uint8_t gray);

/**
 * Convert RGB to RGB565
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return RGB565 color value
 */
ST7789_Color st7789_color565(uint8_t r, uint8_t g, uint8_t b);

/*============================================================================
 * CORE DISPLAY FUNCTIONS
 *==========================================================================*/

/**
 * Initialize ST7789 display
 * Must be called before any other display functions
 */
void st7789_init(void);

/**
 * Set address window for pixel writing
 * Subsequent pixel writes will fill this region
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param w Width of region
 * @param h Height of region
 */
void st7789_setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * Write a single pixel at specified coordinates
 * @param x X coordinate (0-239)
 * @param y Y coordinate (0-239)
 * @param color RGB565 color value
 */
void st7789_writePixel(uint16_t x, uint16_t y, ST7789_Color color);

/**
 * Fill entire screen with solid color
 * @param color RGB565 color value
 */
void st7789_fillScreen(ST7789_Color color);

/**
 * Draw a circle (outline or filled)
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color RGB565 color value
 * @param filled 1 for filled circle, 0 for outline
 */
void st7789_drawCircle(int16_t x0, int16_t y0, int16_t r,
                       ST7789_Color color, uint8_t filled);

/*============================================================================
 * LOW-LEVEL DATA STREAMING (for advanced use)
 *==========================================================================*/

/**
 * Begin data stream (asserts CS, sets DC high)
 * Must call st7789_endData() when done
 */
void st7789_beginData(void);

/**
 * End data stream (deasserts CS)
 */
void st7789_endData(void);

/**
 * Write 16-bit value to display (must be between begin/end data)
 * @param data 16-bit value to write
 */
void st7789_write16(uint16_t data);

#endif // ST7789_DRIVER_H
