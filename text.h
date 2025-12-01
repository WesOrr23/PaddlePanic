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
 */
void drawNumber(uint8_t x, uint8_t y, uint16_t number, OLED_color color);

#endif // TEXT_H
