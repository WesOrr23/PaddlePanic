/*============================================================================
 * st7789_driver.c - Created by Wes Orr (12/8/25)
 *============================================================================
 * Minimal ST7789 240x240 RGB display driver implementation
 * Based on Adafruit ST7789 library, adapted for ATtiny1627
 *
 * HARDWARE CONNECTIONS:
 *     Display Pin    <->    ATtiny1627 Pin
 *     -----------          ---------------
 *     SCL            <->    PC0 (SPI Clock)
 *     SDA            <->    PC2 (SPI Data Out)
 *     RES            <->    PB0 (Reset)
 *     DC             <->    PB1 (Data/Command)
 *     CS             <->    PC3 (Chip Select)
 *     BLK            <->    VCC (3.3V - backlight always on)
 *
 * USAGE:
 *     st7789_init();
 *     st7789_fillScreen(st7789_grayscale(0));  // Clear to black
 *     st7789_drawCircle(120, 120, 30, st7789_grayscale(255), 1);  // White circle
 *==========================================================================*/

// Define CPU frequency for delay functions (ATtiny1627 default is 3.333 MHz)
#ifndef F_CPU
#define F_CPU 3333333UL
#endif

#include "st7789_driver.h"
#include <xc.h>
#include <util/delay.h>

/*============================================================================
 * ST7789 COMMAND DEFINITIONS
 *==========================================================================*/
#define ST7789_NOP        0x00  // No operation
#define ST7789_SWRESET    0x01  // Software reset
#define ST7789_SLPOUT     0x11  // Sleep out
#define ST7789_NORON      0x13  // Normal display mode on
#define ST7789_INVOFF     0x20  // Display inversion off
#define ST7789_INVON      0x21  // Display inversion on
#define ST7789_DISPOFF    0x28  // Display off
#define ST7789_DISPON     0x29  // Display on
#define ST7789_CASET      0x2A  // Column address set
#define ST7789_RASET      0x2B  // Row address set
#define ST7789_RAMWR      0x2C  // Memory write
#define ST7789_MADCTL     0x36  // Memory data access control
#define ST7789_COLMOD     0x3A  // Interface pixel format

// Memory Access Control register bits
#define ST7789_MADCTL_MY  0x80  // Row address order
#define ST7789_MADCTL_MX  0x40  // Column address order
#define ST7789_MADCTL_MV  0x20  // Row/column exchange
#define ST7789_MADCTL_ML  0x10  // Vertical refresh order
#define ST7789_MADCTL_RGB 0x00  // RGB order
#define ST7789_MADCTL_BGR 0x08  // BGR order

/*============================================================================
 * PIN CONTROL MACROS
 *==========================================================================*/
#define DC_LOW()    (PORTB.OUT &= ~0x02)  // DC pin low (command mode)
#define DC_HIGH()   (PORTB.OUT |= 0x02)   // DC pin high (data mode)
#define RES_LOW()   (PORTB.OUT &= ~0x01)  // Reset pin low
#define RES_HIGH()  (PORTB.OUT |= 0x01)   // Reset pin high

/*============================================================================
 * SPI FUNCTIONS (ST7789-specific with proper CS control)
 *==========================================================================*/

/**
 * Send single byte over SPI without CS toggle
 * Assumes CS is already asserted by caller
 */
static void spi_write_byte(uint8_t byte) {
    SPI0.DATA = byte;
    while ((SPI0.INTFLAGS & SPI_IF_bm) == 0) {}  // Wait for transfer complete
}

/**
 * Send command byte to ST7789
 * Handles CS assertion/deassertion
 */
static void st7789_sendCommand(uint8_t cmd) {
    DC_LOW();                     // Command mode
    PORTC.OUT &= ~PIN3_bm;        // Assert CS
    spi_write_byte(cmd);
    PORTC.OUT |= PIN3_bm;         // Deassert CS
}

/**
 * Send command followed by data bytes
 * Keeps CS low for entire transaction
 */
static void st7789_sendCommandWithData(uint8_t cmd, const uint8_t* data, uint8_t len) {
    PORTC.OUT &= ~PIN3_bm;        // Assert CS (stays low for entire transaction)

    DC_LOW();                     // Command mode
    spi_write_byte(cmd);

    DC_HIGH();                    // Data mode
    for (uint8_t i = 0; i < len; i++) {
        spi_write_byte(data[i]);
    }

    PORTC.OUT |= PIN3_bm;         // Deassert CS
}

/**
 * Begin data stream (for pixel writing)
 * Asserts CS and sets DC high - caller must manually deassert CS when done
 */
void st7789_beginData(void) {
    DC_HIGH();
    PORTC.OUT &= ~PIN3_bm;        // Assert CS
}

/**
 * End data stream
 */
void st7789_endData(void) {
    PORTC.OUT |= PIN3_bm;         // Deassert CS
}

/*============================================================================
 * INTERNAL HELPER FUNCTIONS
 *==========================================================================*/

/**
 * Hardware reset sequence
 * Toggles reset pin to reset display controller
 */
static void st7789_hardwareReset(void) {
    RES_HIGH();
    _delay_ms(10);
    RES_LOW();
    _delay_ms(10);
    RES_HIGH();
    _delay_ms(150);  // Wait for reset to complete
}

/**
 * Send 16-bit data value (big-endian)
 * Used for coordinates and color values
 * CS must already be asserted by caller
 * @param data 16-bit value to send
 */
void st7789_write16(uint16_t data) {
    spi_write_byte((uint8_t)(data >> 8));    // High byte
    spi_write_byte((uint8_t)(data & 0xFF));  // Low byte
}

/*============================================================================
 * COLOR CONVERSION FUNCTIONS
 *==========================================================================*/

ST7789_Color st7789_grayscale(uint8_t gray) {
    // Convert 8-bit grayscale to RGB565
    // For grayscale: R = G = B
    uint8_t r = gray >> 3;  // 5 bits (0-31)
    uint8_t g = gray >> 2;  // 6 bits (0-63)
    uint8_t b = gray >> 3;  // 5 bits (0-31)

    return (r << 11) | (g << 5) | b;
}

ST7789_Color st7789_color565(uint8_t r, uint8_t g, uint8_t b) {
    // Convert 8-bit RGB (0-255) to RGB565 format
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/*============================================================================
 * DISPLAY INITIALIZATION
 *==========================================================================*/

void st7789_init(void) {
    // Configure pins as outputs (RES and DC on PORTB)
    // Note: BLK (backlight) should be tied directly to VCC
    PORTB.DIR |= 0x03;  // PB0 (RES), PB1 (DC)

    // Hardware reset
    st7789_hardwareReset();

    // MINIMAL INIT SEQUENCE - bare minimum to wake display
    st7789_sendCommand(ST7789_SWRESET);  // Software reset
    _delay_ms(150);

    st7789_sendCommand(ST7789_SLPOUT);   // Wake from sleep
    _delay_ms(120);

    // Set color mode
    uint8_t colmod_data = 0x05;  // 16-bit RGB565
    st7789_sendCommandWithData(ST7789_COLMOD, &colmod_data, 1);
    _delay_ms(10);

    // Set MADCTL - rotation and color order
    // RGB order (not BGR) for correct colors, no horizontal flip
    uint8_t madctl_data = ST7789_MADCTL_RGB;
    st7789_sendCommandWithData(ST7789_MADCTL, &madctl_data, 1);
    _delay_ms(10);

    // Turn on display
    st7789_sendCommand(ST7789_INVON);    // Inversion ON
    _delay_ms(10);

    st7789_sendCommand(ST7789_NORON);    // Normal display mode
    _delay_ms(10);

    st7789_sendCommand(ST7789_DISPON);   // Display ON
    _delay_ms(120);  // Wait longer for display to stabilize
}

/*============================================================================
 * ADDRESS WINDOW CONTROL
 *==========================================================================*/

void st7789_setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Apply offsets (some displays need this)
    x += ST7789_XSTART;
    y += ST7789_YSTART;

    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    // Set column address (CASET) - keep CS low for entire command+data
    PORTC.OUT &= ~PIN3_bm;      // Assert CS
    DC_LOW();
    spi_write_byte(ST7789_CASET);
    DC_HIGH();
    st7789_write16(x);          // Start column
    st7789_write16(x_end);      // End column
    PORTC.OUT |= PIN3_bm;       // Deassert CS

    // Set row address (RASET) - keep CS low for entire command+data
    PORTC.OUT &= ~PIN3_bm;      // Assert CS
    DC_LOW();
    spi_write_byte(ST7789_RASET);
    DC_HIGH();
    st7789_write16(y);          // Start row
    st7789_write16(y_end);      // End row
    PORTC.OUT |= PIN3_bm;       // Deassert CS

    // Write to RAM command
    st7789_sendCommand(ST7789_RAMWR);
}

/*============================================================================
 * PIXEL DRAWING FUNCTIONS
 *==========================================================================*/

void st7789_writePixel(uint16_t x, uint16_t y, ST7789_Color color) {
    // Bounds check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;

    // Set address window to single pixel
    st7789_setAddrWindow(x, y, 1, 1);

    // Write pixel color (16-bit RGB565) with CS control
    st7789_beginData();
    st7789_write16(color);
    st7789_endData();
}

void st7789_fillScreen(ST7789_Color color) {
    // Set address window to entire screen
    st7789_setAddrWindow(0, 0, ST7789_WIDTH, ST7789_HEIGHT);

    // Write color to all pixels - keep CS low for entire operation
    uint32_t pixel_count = (uint32_t)ST7789_WIDTH * ST7789_HEIGHT;

    st7789_beginData();  // Assert CS once
    for (uint32_t i = 0; i < pixel_count; i++) {
        st7789_write16(color);
    }
    st7789_endData();    // Deassert CS once
}

/*============================================================================
 * CIRCLE DRAWING (Bresenham's Algorithm)
 *==========================================================================*/

/**
 * Draw horizontal line (optimized for filled circles)
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param w Width (length) of line
 * @param color Line color
 */
static void st7789_drawFastHLine(int16_t x, int16_t y, int16_t w,
                                  ST7789_Color color) {
    // Bounds checking
    if (y < 0 || y >= ST7789_HEIGHT) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > ST7789_WIDTH) {
        w = ST7789_WIDTH - x;
    }
    if (w <= 0) return;

    // Set address window for horizontal line
    st7789_setAddrWindow(x, y, w, 1);

    // Write pixels - keep CS low for entire line
    st7789_beginData();
    for (int16_t i = 0; i < w; i++) {
        st7789_write16(color);
    }
    st7789_endData();
}

void st7789_drawCircle(int16_t x0, int16_t y0, int16_t r,
                       ST7789_Color color, uint8_t filled) {
    if (filled) {
        // Draw filled circle using horizontal lines
        int16_t f = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x = 0;
        int16_t y = r;

        // Draw center horizontal line
        st7789_drawFastHLine(x0 - r, y0, 2 * r + 1, color);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            // Draw horizontal lines for each pair of points
            st7789_drawFastHLine(x0 - x, y0 + y, 2 * x + 1, color);
            st7789_drawFastHLine(x0 - x, y0 - y, 2 * x + 1, color);
            st7789_drawFastHLine(x0 - y, y0 + x, 2 * y + 1, color);
            st7789_drawFastHLine(x0 - y, y0 - x, 2 * y + 1, color);
        }
    } else {
        // Draw circle outline using Bresenham's algorithm
        int16_t f = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x = 0;
        int16_t y = r;

        // Draw initial points
        st7789_writePixel(x0, y0 + r, color);
        st7789_writePixel(x0, y0 - r, color);
        st7789_writePixel(x0 + r, y0, color);
        st7789_writePixel(x0 - r, y0, color);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            // Draw 8 octant points
            st7789_writePixel(x0 + x, y0 + y, color);
            st7789_writePixel(x0 - x, y0 + y, color);
            st7789_writePixel(x0 + x, y0 - y, color);
            st7789_writePixel(x0 - x, y0 - y, color);
            st7789_writePixel(x0 + y, y0 + x, color);
            st7789_writePixel(x0 - y, y0 + x, color);
            st7789_writePixel(x0 + y, y0 - x, color);
            st7789_writePixel(x0 - y, y0 - x, color);
        }
    }
}
