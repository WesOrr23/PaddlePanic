/*============================================================================
 * text.c - Created by Wes Orr (12/01/25)
 *============================================================================
 * Simple number rendering implementation
 *==========================================================================*/

#include "text.h"
#include <stddef.h>

/*============================================================================
 * DIGIT BITMAPS (3x5 pixels)
 *==========================================================================*/

/**
 * Digit bitmaps stored as 5 rows of 3 bits each
 * Each row is stored in lowest 3 bits of a uint8_t
 * Bit pattern: 0bXXX where 1=pixel on, 0=pixel off
 *
 * Example for '0':
 *   ###  -> 0b111 = 7
 *   # #  -> 0b101 = 5
 *   # #  -> 0b101 = 5
 *   # #  -> 0b101 = 5
 *   ###  -> 0b111 = 7
 */
static const uint8_t DIGIT_BITMAPS[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
    {0b010, 0b110, 0b010, 0b010, 0b111},  // 1
    {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
    {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
    {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
    {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
    {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
    {0b111, 0b001, 0b001, 0b001, 0b001},  // 7
    {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
    {0b111, 0b101, 0b111, 0b001, 0b111},  // 9
};

/*============================================================================
 * HELPER FUNCTIONS
 *==========================================================================*/

/**
 * Draw a single digit at specified position
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param digit Digit to draw (0-9)
 * @param color Color to draw
 */
static void drawDigit(uint8_t x, uint8_t y, uint8_t digit, OLED_color color) {
    if (digit > 9) return;  // Invalid digit

    // Draw each row of the digit
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
        uint8_t row_data = DIGIT_BITMAPS[digit][row];

        // Draw each pixel in the row (3 bits)
        for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
            if (row_data & (1 << (DIGIT_WIDTH - 1 - col))) {
                drawPixel((Point){x + col, y + row}, color);
            }
        }
    }
}

/*============================================================================
 * PUBLIC FUNCTIONS
 *==========================================================================*/

void drawNumber(uint8_t x, uint8_t y, uint16_t number, OLED_color color) {
    // Handle zero as special case
    if (number == 0) {
        drawDigit(x, y, 0, color);
        return;
    }

    // Extract digits (right to left)
    uint8_t digits[5];  // Max 5 digits for uint16_t (65535)
    uint8_t digit_count = 0;

    uint16_t temp = number;
    while (temp > 0 && digit_count < 5) {
        digits[digit_count++] = temp % 10;
        temp /= 10;
    }

    // Draw digits (left to right, so reverse the array)
    uint8_t current_x = x;
    for (int8_t i = digit_count - 1; i >= 0; i--) {
        drawDigit(current_x, y, digits[i], color);
        current_x += DIGIT_WIDTH + DIGIT_SPACING;
    }
}
