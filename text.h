/*============================================================================
 * text.h - Created by Wes Orr (12/9/25)
 *============================================================================
 * Simple text rendering for ST7789 display
 * 3x5 pixel font, supports A-Z, 0-9, and space
 *==========================================================================*/

#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

/**
 * Draw text string at specified position
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param text String to draw (A-Z, 0-9, space supported)
 * @param gray Grayscale value (0-255)
 * @param scale Scale factor (1=3x5, 2=6x10, etc.)
 */
void st7789_drawText(uint16_t x, uint16_t y, const char* text, uint8_t gray, uint8_t scale);

/**
 * Draw number at specified position
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param number Number to display (0-65535)
 * @param gray Grayscale value (0-255)
 * @param scale Scale factor (1=3x5, 2=6x10, etc.)
 */
void st7789_drawNumber(uint16_t x, uint16_t y, uint16_t number, uint8_t gray, uint8_t scale);

/**
 * Erase text by drawing it in black
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param text String to erase
 * @param scale Scale factor (must match original draw scale)
 */
static inline void st7789_eraseText(uint16_t x, uint16_t y, const char* text, uint8_t scale) {
    st7789_drawText(x, y, text, 0, scale);
}

/**
 * Erase number by drawing it in black
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param number Number to erase
 * @param scale Scale factor (must match original draw scale)
 */
static inline void st7789_eraseNumber(uint16_t x, uint16_t y, uint16_t number, uint8_t scale) {
    st7789_drawNumber(x, y, number, 0, scale);
}

#endif // TEXT_H
