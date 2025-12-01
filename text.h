/*============================================================================
 * text.h - Created by Wes Orr (12/01/25)
 *============================================================================
 * Simple number rendering for SH1106 display
 *
 * Provides minimal text rendering capability for displaying numeric values
 * (score, lives, etc.) using tiny 3x5 pixel digit bitmaps.
 *==========================================================================*/

#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>
#include "sh1106_graphics.h"

/*============================================================================
 * TEXT CONFIGURATION
 *==========================================================================*/

#define DIGIT_WIDTH  3   // Width of each digit in pixels
#define DIGIT_HEIGHT 5   // Height of each digit in pixels
#define DIGIT_SPACING 1  // Space between digits in pixels

/*============================================================================
 * TEXT RENDERING FUNCTIONS
 *==========================================================================*/

/**
 * Draw a number at specified position
 * @param x X position (top-left of first digit)
 * @param y Y position (top-left of first digit)
 * @param number Number to display (0-65535)
 * @param color Color to draw (COLOR_WHITE, COLOR_BLACK, or COLOR_INVERT)
 * @param scale Scale factor (1=3x5, 2=6x10, 3=9x15, etc.)
 */
void drawNumber(uint8_t x, uint8_t y, uint16_t number, OLED_color color, uint8_t scale);

/**
 * Draw text at specified position
 * Supports uppercase/lowercase letters (A-Z), digits (0-9), and spaces
 * @param x X position (top-left of first character)
 * @param y Y position (top-left of first character)
 * @param text Null-terminated string to display
 * @param color Color to draw (COLOR_WHITE, COLOR_BLACK, or COLOR_INVERT)
 * @param scale Scale factor (1=3x5, 2=6x10, 3=9x15, etc.)
 */
void drawText(uint8_t x, uint8_t y, const char* text, OLED_color color, uint8_t scale);

#endif // TEXT_H
