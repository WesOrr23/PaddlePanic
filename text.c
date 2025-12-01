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
 * LETTER BITMAPS (3x5 pixels)
 *==========================================================================*/

/**
 * Letter bitmaps for A-Z (uppercase only)
 * Same format as digits: 5 rows of 3 bits each
 */
static const uint8_t LETTER_BITMAPS[26][5] = {
    {0b111, 0b101, 0b111, 0b101, 0b101},  // A
    {0b110, 0b101, 0b110, 0b101, 0b110},  // B
    {0b111, 0b100, 0b100, 0b100, 0b111},  // C
    {0b110, 0b101, 0b101, 0b101, 0b110},  // D
    {0b111, 0b100, 0b111, 0b100, 0b111},  // E
    {0b111, 0b100, 0b111, 0b100, 0b100},  // F
    {0b111, 0b100, 0b101, 0b101, 0b111},  // G
    {0b101, 0b101, 0b111, 0b101, 0b101},  // H
    {0b111, 0b010, 0b010, 0b010, 0b111},  // I
    {0b111, 0b001, 0b001, 0b101, 0b111},  // J
    {0b101, 0b110, 0b100, 0b110, 0b101},  // K
    {0b100, 0b100, 0b100, 0b100, 0b111},  // L
    {0b101, 0b111, 0b111, 0b101, 0b101},  // M
    {0b101, 0b111, 0b111, 0b111, 0b101},  // N
    {0b111, 0b101, 0b101, 0b101, 0b111},  // O
    {0b111, 0b101, 0b111, 0b100, 0b100},  // P
    {0b111, 0b101, 0b101, 0b111, 0b001},  // Q
    {0b111, 0b101, 0b110, 0b101, 0b101},  // R
    {0b111, 0b100, 0b111, 0b001, 0b111},  // S
    {0b111, 0b010, 0b010, 0b010, 0b010},  // T
    {0b101, 0b101, 0b101, 0b101, 0b111},  // U
    {0b101, 0b101, 0b101, 0b101, 0b010},  // V
    {0b101, 0b101, 0b111, 0b111, 0b101},  // W
    {0b101, 0b101, 0b010, 0b101, 0b101},  // X
    {0b101, 0b101, 0b010, 0b010, 0b010},  // Y
    {0b111, 0b001, 0b010, 0b100, 0b111},  // Z
};

/*============================================================================
 * HELPER FUNCTIONS
 *==========================================================================*/

/**
 * Draw a single character at specified position
 * Handles uppercase letters (A-Z), digits (0-9), and space
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param c Character to draw
 * @param color Color to draw
 * @param scale Scale factor (1=normal, 2=2x, etc.)
 */
static void drawChar(uint8_t x, uint8_t y, char c, OLED_color color, uint8_t scale) {
    if (scale == 0) scale = 1;  // Prevent divide by zero

    const uint8_t* bitmap = NULL;

    // Determine which bitmap to use
    if (c >= '0' && c <= '9') {
        bitmap = DIGIT_BITMAPS[c - '0'];
    } else if (c >= 'A' && c <= 'Z') {
        bitmap = LETTER_BITMAPS[c - 'A'];
    } else if (c >= 'a' && c <= 'z') {
        bitmap = LETTER_BITMAPS[c - 'a'];  // Convert lowercase to uppercase
    } else if (c == ' ') {
        return;  // Space - just skip (spacing handled by caller)
    } else {
        return;  // Unsupported character
    }

    // Draw each row of the character
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
        uint8_t row_data = bitmap[row];

        // Draw each pixel in the row (3 bits)
        for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
            if (row_data & (1 << (DIGIT_WIDTH - 1 - col))) {
                // Draw scaled pixel (scale x scale block)
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        drawPixel((Point){x + col * scale + sx, y + row * scale + sy}, color);
                    }
                }
            }
        }
    }
}

/**
 * Draw a single digit at specified position
 * @param x X position (top-left)
 * @param y Y position (top-left)
 * @param digit Digit to draw (0-9)
 * @param color Color to draw
 * @param scale Scale factor (1=normal, 2=2x, etc.)
 */
static void drawDigit(uint8_t x, uint8_t y, uint8_t digit, OLED_color color, uint8_t scale) {
    if (digit > 9) return;  // Invalid digit
    drawChar(x, y, '0' + digit, color, scale);
}

/*============================================================================
 * PUBLIC FUNCTIONS
 *==========================================================================*/

void drawNumber(uint8_t x, uint8_t y, uint16_t number, OLED_color color, uint8_t scale) {
    if (scale == 0) scale = 1;  // Prevent issues

    // Handle zero as special case
    if (number == 0) {
        drawDigit(x, y, 0, color, scale);
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
    uint8_t scaled_width = DIGIT_WIDTH * scale;
    uint8_t scaled_spacing = DIGIT_SPACING * scale;

    for (int8_t i = digit_count - 1; i >= 0; i--) {
        drawDigit(current_x, y, digits[i], color, scale);
        current_x += scaled_width + scaled_spacing;
    }
}

void drawText(uint8_t x, uint8_t y, const char* text, OLED_color color, uint8_t scale) {
    if (text == NULL || scale == 0) return;
    if (scale == 0) scale = 1;

    uint8_t current_x = x;
    uint8_t scaled_width = DIGIT_WIDTH * scale;
    uint8_t scaled_spacing = DIGIT_SPACING * scale;

    // Draw each character
    for (uint8_t i = 0; text[i] != '\0'; i++) {
        drawChar(current_x, y, text[i], color, scale);
        current_x += scaled_width + scaled_spacing;
    }
}
