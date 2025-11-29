/*----------------------------------------------------------------------------
// ACU_SH1106_gfxlib.h    		 version 1 created by Darby Hewitt 10/27/2024
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

//
// To set up the hardware:
// 1) using an ATtiny1627 on a dev board such as the ATtiny1627 Curiosity Nano
//    along with a 128x64-pixel SH1106-driven OLED display
//	  make the following connections:
//
//     Display	<->		ATtiny1627
//	   CLK		<->		PC0
//	   MOSI		<->		PC2
//	   RES		<->		PB0
//	   DC		<->		PB1
//	   CS		<->		PC3
//
// 2) in main(), run initSPI(), followed by initScreen()
//
// After these steps are performed, you should be able to run any of the 
// write... commands followed by showScreen() and see your objects on the 
// display. 
//
// TO DO:
//  -make library more configurable 
//		(i.e., not tightly bound to specific pins)
//  -improve efficiency of write commands when possible
//		(i.e., avoid use of "writePixel()" for lines and shapes)
//	-more robust/existent font support
//
// Good luck, y'all! 
//------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------

The following is copied from gfxfont.h in the Adafruit_GFX library


--------------------------------------------------------------------------*/

// Font structures for newer Adafruit_GFX (1.1 and later).
// Example fonts are included in 'Fonts' directory.
// To use a font in your Arduino sketch, #include the corresponding .h
// file and pass address of GFXfont struct to setFont().  Pass NULL to
// revert to 'classic' fixed-space bitmap font.

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

/// Font data stored PER GLYPH
typedef struct {
	uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
	uint8_t width;         ///< Bitmap dimensions in pixels
	uint8_t height;        ///< Bitmap dimensions in pixels
	uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
	int8_t xOffset;        ///< X dist from cursor pos to UL corner
	int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct {
	uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
	GFXglyph *glyph;  ///< Glyph array
	uint16_t first;   ///< ASCII extents (first char)
	uint16_t last;    ///< ASCII extents (last char)
	uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

#endif // _GFXFONT_H_
/*--------------------------------------------------------------------------------
end of gfxfont.h copy from Adafruit_GFX library
---------------------------------------------------------------------------------*/


#define WIDTH	128
#define HEIGHT	64

#define GRAYOLED_NORMALDISPLAY 0xA6 ///< Generic non-invert for almost all OLEDs
#define GRAYOLED_INVERTDISPLAY 0xA7 ///< Generic invert for almost all OLEDs

typedef enum OLED_colors{
	COLOR_BLACK, 
	COLOR_WHITE,
	COLOR_INVERT
	} OLED_color; //just to clarify down below

void initSPI();
void sendByteSPI(uint8_t byteToSend);
void sendTwoBytesSPI(uint8_t bytesToSend[2]);
void sendCommand(uint8_t commByte);
void sendData(uint8_t dataByte);
void initScreen();

//------------------------------------------------
//Drawing functions
// these are heavily inspired by/copied from 
// Adafruit_GFX 
//
void writePixel(uint8_t x, uint8_t y, OLED_color color);

void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, OLED_color color);

void writeRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_color color);
void writeFilledRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_color color);

void writeCircle(int16_t x0, int16_t y0, int16_t r, OLED_color color);
void writeQuarterCircle(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, OLED_color color);
void writeFilledCircle(int16_t x0, int16_t y0, int16_t r, OLED_color color);
//you could use this, but probably don't need to
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, OLED_color color);


void writeTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, OLED_color color);
void writeFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, OLED_color color);

//TO DO: remove the two functions below eventually?
void drawFastVLine(int16_t x, int16_t y, int16_t h, OLED_color color);
void drawFastHLine(int16_t x, int16_t y, int16_t W, OLED_color color);

void writeBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, OLED_color color);

//font and text stuff
void setTextSize(uint8_t s_x, uint8_t s_y);
void setFont(const GFXfont *f);
void setTextColor(OLED_color bg, OLED_color fg);
void setTextCursor(uint8_t x_cursor, uint8_t y_cursor);
void drawChar(int16_t x, int16_t y, unsigned char c, OLED_color color, OLED_color bg, uint8_t size_x, uint8_t size_y);
size_t writeSingle(uint8_t c);
size_t write(const char *buffer, size_t size);

// text helpers
void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);
void getTextBounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);


//clear buffer; doesn't actually access screen, but erases what will be written
void clearDisplay(void);

//------------------------------------------------
//Pixel getter -- gets pixel from buffer
uint8_t getPixel(int16_t x, int16_t y);

//------------------------------------------------
//Display Manipulation Functions
void invertDisplay(uint8_t inv);
void showScreen();



