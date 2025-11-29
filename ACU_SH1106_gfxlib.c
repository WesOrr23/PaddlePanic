/*----------------------------------------------------------------------------
// ACU_SH1106_gfxlib.c 				version 1 by Darby Hewitt 10/27/24
//----------------------------------------------------------------------------
// Simple graphics and command library for a SH1106-driven, B&W, 
// 128x64-pixel OLED display, accessed by the ATtiny1627, and used in
// ECEN 315/316 - "Introduction to Embedded Systems" at ACU.
// 
// This library is provided by Dr. Darby Hewitt and it is heavily inspired 
// by/copied from both the Adafruit_GFX and Adafruit_GrayOLED libraries.
//
// The above Adafruit libaries are disrtibuted with the follwing preamble:

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

//--------------------------------------------------------------------------*/

#include <xc.h>

#include "ACU_SH1106_gfxlib.h"

#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

//macros here taken from Adafruit_GFX library; used in various functions
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
{                                                                            \
	int16_t t = a;                                                             \
	a = b;                                                                     \
	b = t;                                                                     \
}
#endif

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
#define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

//display buffer
uint8_t buffer[WIDTH * ((HEIGHT + 7) / 8)] = {0}; //initialize to zeros

//other key variables for the graphics (specifically text-related) functions
int16_t cursor_x = 10;     ///< x location to start print()ing text
int16_t cursor_y = 10;     ///< y location to start print()ing text
OLED_color textcolor = COLOR_WHITE;   ///< 16-bit background color for print()
OLED_color textbgcolor = COLOR_BLACK; ///< 16-bit text color for print()
uint8_t textsize_x;   ///< Desired magnification in X-axis of text to print()
uint8_t textsize_y;   ///< Desired magnification in Y-axis of text to print()
uint8_t wrap = 1;            ///< If set, 'wrap' text at right edge of display
uint8_t _cp437 = 0;          ///< If set, use correct CP437 charset (default is off)
GFXfont *gfxFont;


/*-----------------------------------------------------------------
//the two inline functions below are straight from Adafruit_GFX
*/
inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
	#ifdef __AVR__
	return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
	#else
	// expression in __AVR__ section may generate "dereferencing type-punned
	// pointer will break strict-aliasing rules" warning In fact, on other
	// platforms (such as STM32) there is no need to do this pointer magic as
	// program memory may be read in a usual way So expression may be simplified
	return gfxFont->glyph + c;
	#endif //__AVR__
}

inline uint8_t *pgm_read_bitmap_ptr(const GFXfont *gfxFont) {
	#ifdef __AVR__
	return (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);
	#else
	// expression in __AVR__ section generates "dereferencing type-punned pointer
	// will break strict-aliasing rules" warning In fact, on other platforms (such
	// as STM32) there is no need to do this pointer magic as program memory may
	// be read in a usual way So expression may be simplified
	return gfxFont->bitmap;
	#endif //__AVR__
}
//---------------------------------------------------------------------


void initSPI(){
	
	//Set SPI pin routing to ALT in PORTMUX:
	PORTMUX.SPIROUTEA |= PORTMUX_SPI0_ALT1_gc;
	
	//SPI setup steps from datasheet:
	//1. Configure the SPI pins to output in the port peripheral
	PORTC.DIR |= PIN0_bm|PIN2_bm|PIN3_bm; //PC2->MOSI, PC0->SCLK, PC3->SS
	
	
	//2. Select the SPI host/client operation by writing the Host/Client Select (MASTER) bit in the Control A (SPIn.CTRLA) register.
	//3. In Host mode, select the clock speed by writing the Prescaler (PRESC) bits and the Clock Double (CLK2X) bit in SPIn.CTRLA.
	SPI0.CTRLA |= SPI_MASTER_bm|SPI_CLK2X_bm|SPI_PRESC_DIV16_gc; //PRESC bits zeroed out correspond to div by 4
	
	//4. Optional: Select the Data Transfer mode by writing to the MODE bits in the Control B (SPIn.CTRLB) register.
	SPI0.CTRLB |= SPI_MODE_3_gc;
	//5. Optional: Write the Data Order (DORD) bit in SPIn.CTRLA.
	//6. Optional: Set up the Buffer mode by writing the BUFEN and BUFWR bits in the Control B (SPIn.CTRLB) register.
	
	//7. Optional: To disable the multi-host support in Host mode, write ‘1’ to the Client Select Disable (SSD) bit in SPIn.CTRLB.
	//8. Enable the SPI by writing a ‘1’ to the ENABLE bit in SPIn.CTRLA.
	SPI0.CTRLA |= SPI_ENABLE_bm;
	
	PORTC.OUTSET = PIN3_bm;

}

void sendByteSPI(uint8_t byteToSend){
	//write to DATA
	SPI0.DATA = byteToSend;
	//set CS pin low
	PORTC.OUT &= ~PIN3_bm;
	while((SPI0.INTFLAGS&SPI_IF_bm) == 0){}
	//set CS pin high
	PORTC.OUT |= PIN3_bm;
	return;
}

void sendTwoBytesSPI(uint8_t bytesToSend[2]){
	sendByteSPI(bytesToSend[0]);
	while((SPI0.INTFLAGS&SPI_IF_bm) == 0){}
	SPI0.INTFLAGS |= SPI_TXCIE_bm; //clear the interrupt
	sendByteSPI(bytesToSend[1]);
	while((SPI0.INTFLAGS&SPI_IF_bm) == 0){}
	return;
}

void sendCommand(uint8_t commByte){
	//set d/~c pin (PB1) to LOW
	PORTB.OUT &= ~0x02;
	sendByteSPI(commByte);
}

void sendData(uint8_t dataByte){
	//set d/~c pin (PB1) to LOW
	PORTB.OUT |= 0x02;
	sendByteSPI(dataByte);
}

void initScreen(){
	 // bitmap framebuffer 'buffer' has been preallocated
	 
	
	//Our screen uses the SH1106 chipset; the following comes
	// from 1.2 of the datasheet for SH1106:
	
	// PB0: RES: reset
	// PB1: D/~C: dataRAM (1) or command (0)
	PORTB.DIR |= PIN0_bm|PIN1_bm;
	
	// PA1: MOSI
	// PA3: SCK
	// PA4: ~CS
	
	//1. Keep RES pin LOW for >~10us 
	//2. Release RES (HI)
	PORTB.OUT |= PIN0_bm; //reset high
	for(uint8_t i = 0; i<250; i++){}
	PORTB.OUT &= ~PIN0_bm; //reset low
	for(uint16_t i = 0; i<1000; i++){}
	PORTB.OUT |= PIN0_bm; //reset high
	
	uint8_t initSequence[] = {0xAE, 0xD5, 0x80, 0xA8, 
							  0x3F, 0xD3, 0x00, 0x40, 
							  0xAD, 0x8B, 0xA1, 0xC8, 
							  0xDA, 0x12, 0x81, 0xFF, 
							  0xD9, 0x1F, 0xDB, 0x40, 
							  0x33, 0xA6, 0x20, 0x00, 
							  0x10, 0xA4};
	
	for(uint8_t i = 0; i<(sizeof(initSequence)/sizeof(initSequence[0])); i++){
		sendCommand(initSequence[i]);
	}
	
	for(uint16_t i = 0; i<40000; i++){ } //wait a little
	
	sendCommand(0xAF); //disp ON
	
	return;
}

// DRAWING FUNCTIONS -------------------------------------------------------

/* Heavily borrowed from Adafruit_GrayOLED library with these
*/
void writePixel(uint8_t x, uint8_t y, OLED_color color) {
  if ((x < WIDTH) && (y < HEIGHT)) {
    // Pixel is in-bounds.
    switch (color) {
    case COLOR_WHITE:
    buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
    break;
    case COLOR_BLACK:
    buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
    break;
    case COLOR_INVERT:
    buffer[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
    break;
    }
  }
}

//writeLine copied from Adafruit_GFX library
void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_color color) {
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1) {
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
		} else {
		ystep = -1;
	}

	for (; x0 <= x1; x0++) {
		if (steep) {
			writePixel(y0, x0, color);
			} else {
			writePixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}
void writeRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_color color){
	//inspired by Adafruit_GFX library
	//draw rectangle in buffer with top left and bottom right corners defined by (x1, y1) and (x2, y2)
	
	//four lines:
	writeLine(x1, y1, x2, y1, color);
	writeLine(x2, y1, x2, y2, color);
	writeLine(x2, y2, x1, y2, color);
	writeLine(x1, y2, x1, y1, color);
}

void writeFilledRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_color color){
	//inspired by Adafruit_GFX library
	//draw filled rectangle in buffer with top left and bottom right corners defined by (x1, y1) and (x2, y2)
	//adjust so (x1,y1) is above and to the left of (x2, y2) if necessary
	if(x2 < x1){
		uint8_t xbuf = x1;
		x1 = x2;
		x2 = xbuf;
	}
	
	if(y2 < y1){
		uint8_t ybuf = y1;
		y1 = y2;
		y2 = ybuf;
	}
	
	for( ; x1<=x2 ; x1++){
		writeLine(x1, y1, x1, y2, color); //write horizontal lines until we're done
	}
}

//the following two functions are needed for fillCircle, fillTriangle, and some others
void drawFastVLine(int16_t x, int16_t y, int16_t h, OLED_color color) {
  writeLine(x, y, x, y + h - 1, color);
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, OLED_color color) {
  writeLine(x, y, x + w - 1, y, color);
}

void writeCircle(int16_t x0, int16_t y0, int16_t r, OLED_color color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  
  writePixel(x0, y0 + r, color);
  writePixel(x0, y0 - r, color);
  writePixel(x0 + r, y0, color);
  writePixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    writePixel(x0 + x, y0 + y, color);
    writePixel(x0 - x, y0 + y, color);
    writePixel(x0 + x, y0 - y, color);
    writePixel(x0 - x, y0 - y, color);
    writePixel(x0 + y, y0 + x, color);
    writePixel(x0 - y, y0 + x, color);
    writePixel(x0 + y, y0 - x, color);
    writePixel(x0 - y, y0 - x, color);
  }
}

void writeQuarterCircle(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, OLED_color color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      writePixel(x0 + x, y0 + y, color);
      writePixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      writePixel(x0 + x, y0 - y, color);
      writePixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      writePixel(x0 - y, y0 + x, color);
      writePixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      writePixel(x0 - y, y0 - x, color);
      writePixel(x0 - x, y0 - y, color);
    }
  }
}

void writeTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, OLED_color color) {
  writeLine(x0, y0, x1, y1, color);
  writeLine(x1, y1, x2, y2, color);
  writeLine(x2, y2, x0, y0, color);
}

void writeFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, OLED_color color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  if (y1 > y2) {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
  }
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  
  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    drawFastHLine(a, y0, b - a + 1, color);
    
    return;
  }

  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
    last = y1; // Include y1 scanline
  else
    last = y1 - 1; // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }
 
}

void writeFilledCircle(int16_t x0, int16_t y0, int16_t r, OLED_color color) {
  
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
  
}

void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, OLED_color color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void writeBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, OLED_color color) {

	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t b = 0;

	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
			b <<= 1;
			else
			b = bitmap[j * byteWidth + i / 8];
			if (b & 0x80)
			writePixel(x + i, y, color);
		}
	}
}

//font stuff
void setTextSize(uint8_t s_x, uint8_t s_y) {
	textsize_x = (s_x > 0) ? s_x : 1;
	textsize_y = (s_y > 0) ? s_y : 1;
}

void setFont(const GFXfont *f) {
	if (f) {          // Font struct pointer passed in?
		if (!gfxFont) { // And no current font struct?
			// Switching from classic to new font behavior.
			// Move cursor pos down 6 pixels so it's on baseline.
			cursor_y += 6;
		}
	} else if (gfxFont) { // NULL passed.  Current font struct defined?
		// Switching from new to classic font behavior.
		// Move cursor pos up 6 pixels so it's at top-left of char.
		cursor_y -= 6;
	}
	gfxFont = (GFXfont *)f;
}

void setTextColor(OLED_color bg, OLED_color fg){
	textbgcolor = bg;
	textcolor = fg;
}


void setTextCursor(uint8_t x_cursor, uint8_t y_cursor){
	cursor_x = x_cursor;
	cursor_y = y_cursor;
}

void drawChar(int16_t x, int16_t y, unsigned char c, OLED_color color, OLED_color bg, uint8_t size_x, uint8_t size_y) {

	//if (!gfxFont) { // 'Classic' built-in font
//
		//if ((x >= WIDTH) ||              // Clip right
		//(y >= HEIGHT) ||             // Clip bottom
		//((x + 6 * size_x - 1) < 0) || // Clip left
		//((y + 8 * size_y - 1) < 0))   // Clip top
		//return;
//
		//if (!_cp437 && (c >= 176))
			//c++; // Handle 'classic' charset behavior
//
		//for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns
			//uint8_t line = pgm_read_byte(&font[c * 5 + i]);
			//for (int8_t j = 0; j < 8; j++, line >>= 1) {
				//if (line & 1) {
					//if (size_x == 1 && size_y == 1)
						//writePixel(x + i, y + j, color);
					//else
						//writeFilledRect(x + i * size_x, y + j * size_y, size_x, size_y, color);
				//} else if (bg != color) {
					//if (size_x == 1 && size_y == 1)
						//writePixel(x + i, y + j, bg);
					//else
						//writeFilledRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
				//}
			//}
		//}
		//if (bg != color) { // If opaque, draw vertical line for last column
			//if (size_x == 1 && size_y == 1)
				//drawFastVLine(x + 5, y, 8, bg);
			//else
				//writeFilledRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
		//}
//
	//} else { // Custom font

	// Character is assumed previously filtered by write() to eliminate
	// newlines, returns, non-printable characters, etc.  Calling
	// drawChar() directly with 'bad' characters of font may cause mayhem!

	c -= (uint8_t)pgm_read_byte(&gfxFont->first);
	GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
	uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

	uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
	uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
	int8_t xo = pgm_read_byte(&glyph->xOffset),
	yo = pgm_read_byte(&glyph->yOffset);
	uint8_t xx, yy, bits = 0, bit = 0;
	int16_t xo16 = 0, yo16 = 0;

	if (size_x > 1 || size_y > 1) {
		xo16 = xo;
		yo16 = yo;
	}

	// Todo: Add character clipping here

	// NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
	// THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
	// has typically been used with the 'classic' font to overwrite old
	// screen contents with new data.  This ONLY works because the
	// characters are a uniform size; it's not a sensible thing to do with
	// proportionally-spaced fonts with glyphs of varying sizes (and that
	// may overlap).  To replace previously-drawn text when using a custom
	// font, use the getTextBounds() function to determine the smallest
	// rectangle encompassing a string, erase the area with fillRect(),
	// then draw new text.  This WILL unfortunately 'blink' the text, but
	// is unavoidable.  Drawing 'background' pixels will NOT fix this,
	// only creates a new set of problems.  Have an idea to work around
	// this (a canvas object type for MCUs that can afford the RAM and
	// displays supporting setAddrWindow() and pushColors()), but haven't
	// implemented this yet.

	for (yy = 0; yy < h; yy++) {
		for (xx = 0; xx < w; xx++) {
			if (!(bit++ & 7)) {
				bits = pgm_read_byte(&bitmap[bo++]);
			}
			if (bits & 0x80) {
				if (size_x == 1 && size_y == 1) {
					writePixel(x + xo + xx, y + yo + yy, color);
					} else {
					writeFilledRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y,
					size_x, size_y, color);
				}
			}
			bits <<= 1;
		}
	}

//	} // End classic vs custom font
}

size_t writeSingle(uint8_t c) {
	if (!gfxFont) { // 'Classic' built-in font

		if (c == '\n') {              // Newline?
			cursor_x = 0;               // Reset x to zero,
			cursor_y += textsize_y * 8; // advance y one line
			} else if (c != '\r') {       // Ignore carriage returns
			if (wrap && ((cursor_x + textsize_x * 6) > WIDTH)) { // Off right?
				cursor_x = 0;                                       // Reset x to zero,
				cursor_y += textsize_y * 8; // advance y one line
			}
			drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
			textsize_y);
			cursor_x += textsize_x * 6; // Advance x one char
		}

	} else { // Custom font

		if (c == '\n') {
			cursor_x = 0;
			cursor_y +=
			(int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
		} else if (c != '\r') {
			uint8_t first = pgm_read_byte(&gfxFont->first);
			if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
				GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
				uint8_t w = pgm_read_byte(&glyph->width),
				h = pgm_read_byte(&glyph->height);
				if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
					int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
					if (wrap && ((cursor_x + textsize_x * (xo + w)) > WIDTH)) {
						cursor_x = 0;
						cursor_y += (int16_t)textsize_y *(uint8_t)pgm_read_byte(&gfxFont->yAdvance);
					}
					drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,	textsize_y);
				}
				cursor_x +=	(uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
			}
		}
	}
	return 1;
}

size_t write(const char *msg, size_t size){
	size_t n = 0;
	while (size>0) {
		if (writeSingle(*msg++)){
			 n++;
			 size--;
		}
		else break;
	}
	return n;
}

void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {

	if (gfxFont) {

		if (c == '\n') { // Newline?
			*x = 0;        // Reset x to zero, advance y by one line
			*y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
		} else if (c != '\r') { // Not a carriage return; is normal char
			uint8_t first = pgm_read_byte(&gfxFont->first),
			last = pgm_read_byte(&gfxFont->last);
			if ((c >= first) && (c <= last)) { // Char present in this font?
				GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
				uint8_t gw = pgm_read_byte(&glyph->width),
				gh = pgm_read_byte(&glyph->height),
				xa = pgm_read_byte(&glyph->xAdvance);
				int8_t xo = pgm_read_byte(&glyph->xOffset),
				yo = pgm_read_byte(&glyph->yOffset);
				if (wrap && ((*x + (((int16_t)xo + gw) * textsize_x)) > WIDTH)) {
					*x = 0; // Reset x to zero, advance y by one line
					*y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
				}
				int16_t tsx = (int16_t)textsize_x, tsy = (int16_t)textsize_y,
				x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + gw * tsx - 1,
				y2 = y1 + gh * tsy - 1;
				if (x1 < *minx)
					*minx = x1;
				if (y1 < *miny)
					*miny = y1;
				if (x2 > *maxx)
					*maxx = x2;
				if (y2 > *maxy)
					*maxy = y2;
				*x += xa * tsx;
			}
		}

	} else { // Default font

		if (c == '\n') {        // Newline?
			*x = 0;               // Reset x to zero,
			*y += textsize_y * 8; // advance y one line
			// min/max x/y unchaged -- that waits for next 'normal' character
			} else if (c != '\r') { // Normal char; ignore carriage returns
			if (wrap && ((*x + textsize_x * 6) > WIDTH)) { // Off right?
				*x = 0;                                       // Reset x to zero,
				*y += textsize_y * 8;                         // advance y one line
			}
			int x2 = *x + textsize_x * 6 - 1, // Lower-right pixel of char
			y2 = *y + textsize_y * 8 - 1;
			if (x2 > *maxx)
			*maxx = x2; // Track max x, y
			if (y2 > *maxy)
			*maxy = y2;
			if (*x < *minx)
			*minx = *x; // Track min x, y
			if (*y < *miny)
			*miny = *y;
			*x += textsize_x * 6; // Advance x one char
		}
	}
}

void getTextBounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {

	uint8_t c; // Current character
	int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1; // Bound rect
	// Bound rect is intentionally initialized inverted, so 1st char sets it

	*x1 = x; // Initial position is value passed in
	*y1 = y;
	*w = *h = 0; // Initial size is zero

	while ((c = *str++)) {
		// charBounds() modifies x/y to advance for each character,
		// and min/max x/y are updated to incrementally build bounding rect.
		charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
	}

	if (maxx >= minx) {     // If legit string bounds were found...
		*x1 = minx;           // Update x1 to least X coord,
		*w = maxx - minx + 1; // And w to bound rect width
	}
	if (maxy >= miny) { // Same for height
		*y1 = miny;
		*h = maxy - miny + 1;
	}
}

/*
    Clear contents of display buffer (set all pixels to off).
*/
void clearDisplay(void) {
  memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
}

uint8_t getPixel(int16_t x, int16_t y) {
  if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
    // Pixel is in-bounds
    return (buffer[x + (y / 8) * WIDTH] & (1 << (y & 7)));
  }
  return 0; // Pixel out of bounds
}

void invertDisplay(uint8_t inv) {
	if(inv){
	  sendCommand(GRAYOLED_INVERTDISPLAY);
	}
	else{
	  sendCommand(GRAYOLED_NORMALDISPLAY);
	}
}

void showScreen(){
	//write all contents of buffer to screen

	//for each of the 8 page addresses (rows of 8-bit columns), 
	for(uint8_t p = 0 ; p < 8 ; p++){
		sendCommand((0xB0|p));//on page p now

		//First, send Read-Modify-Write command:
		sendCommand(0xE0);
		//now, send 128 bytes of data sequentially:
		for(uint8_t i = 0; i<2; i++){
			sendData(0x00);
		}
		for(uint8_t i = 0 ; i<128; i++){
			sendData(buffer[(p*128)+i]);
		}
		//finally, send "End" command
		sendCommand(0xEE);
	}
}