/*============================================================================
 * st7789_text.c - Created by Wes Orr (12/9/25)
 *============================================================================
 * Simple text rendering implementation for ST7789
 * Ported from text.c for SH1106
 *==========================================================================*/

#include "st7789_text.h"
#include "st7789_driver.h"
#include <stddef.h>

/*============================================================================
 * FONT BITMAPS (3x5 pixels)
 *==========================================================================*/

// Digit bitmaps (0-9)
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

// Letter bitmaps (A-Z)
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
 * Draw a single character
 */
static void drawChar(uint16_t x, uint16_t y, char c, uint8_t gray, uint8_t scale) {
    if (scale == 0) scale = 1;

    const uint8_t* bitmap = NULL;

    // Determine which bitmap to use
    if (c >= '0' && c <= '9') {
        bitmap = DIGIT_BITMAPS[c - '0'];
    } else if (c >= 'A' && c <= 'Z') {
        bitmap = LETTER_BITMAPS[c - 'A'];
    } else if (c >= 'a' && c <= 'z') {
        bitmap = LETTER_BITMAPS[c - 'a'];
    } else if (c == ' ') {
        // Space - just skip width
        return;
    } else {
        // Unsupported character - skip
        return;
    }

    // Render the bitmap
    ST7789_Color color = st7789_grayscale(gray);

    for (uint8_t row = 0; row < 5; row++) {
        uint8_t row_data = bitmap[row];

        for (uint8_t col = 0; col < 3; col++) {
            if (row_data & (1 << (2 - col))) {  // Check bit MSB first (bit 2 = leftmost pixel)
                // Draw scaled pixel block
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        st7789_writePixel(x + col * scale + sx,
                                         y + row * scale + sy,
                                         color);
                    }
                }
            }
        }
    }
}

/*============================================================================
 * PUBLIC FUNCTIONS
 *==========================================================================*/

void st7789_drawText(uint16_t x, uint16_t y, const char* text, uint8_t gray, uint8_t scale) {
    if (text == NULL || scale == 0) return;

    uint16_t cursor_x = x;
    uint8_t char_width = 3 * scale + scale;  // 3 pixels + 1 spacing

    while (*text) {
        if (*text == ' ') {
            cursor_x += char_width;  // Space width
        } else {
            drawChar(cursor_x, y, *text, gray, scale);
            cursor_x += char_width;
        }
        text++;
    }
}

void st7789_drawNumber(uint16_t x, uint16_t y, uint16_t number, uint8_t gray, uint8_t scale) {
    if (scale == 0) scale = 1;

    // Convert number to string (max 5 digits for uint16_t)
    char buffer[6];
    uint8_t i = 0;

    // Handle zero special case
    if (number == 0) {
        buffer[i++] = '0';
    } else {
        // Extract digits (reverse order)
        uint16_t temp = number;
        while (temp > 0) {
            buffer[i++] = '0' + (temp % 10);
            temp /= 10;
        }

        // Reverse the digits
        for (uint8_t j = 0; j < i / 2; j++) {
            char tmp = buffer[j];
            buffer[j] = buffer[i - 1 - j];
            buffer[i - 1 - j] = tmp;
        }
    }

    buffer[i] = '\0';  // Null terminate

    // Draw the string
    st7789_drawText(x, y, buffer, gray, scale);
}
