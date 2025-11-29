/*============================================================================
 * shapes.c - Created by Wes Orr (11/29/25)
 *============================================================================
 * Object-oriented shape abstraction layer for SH1106 graphics library
 * 
 * Implements polymorphic shape interface and all shape-specific drawing
 * code (circles, rectangles) using primitives from sh1106_graphics.c
 *==========================================================================*/

#include "shapes.h"
#include "sh1106_graphics.h"
#include <stdlib.h>
#include <stddef.h>

/*============================================================================
 * INTERNAL MACROS
 *==========================================================================*/
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

/*============================================================================
 * CIRCLE DRAWING PRIMITIVES
 *==========================================================================*/

/**
 * Draw a circle outline using midpoint circle algorithm
 * Uses 8-way symmetry to draw entire circle by calculating only 1/8 of it
 */
static void writeCircle(Point center, int16_t radius, OLED_color color) {
    int16_t centerX = center.x;
    int16_t centerY = center.y;
    
    // Midpoint circle algorithm variables
    int16_t decisionParam = 1 - radius;         // Decision parameter for next pixel
    int16_t deltaDecisionX = 1;                 // Change in decision when x increases
    int16_t deltaDecisionY = -2 * radius;       // Change in decision when y decreases
    int16_t x = 0;                              // Start at leftmost point
    int16_t y = radius;                         // Start at top
    
    // Draw initial cardinal points (top, bottom, left, right)
    writePixel((Point){centerX, centerY + radius}, color);
    writePixel((Point){centerX, centerY - radius}, color);
    writePixel((Point){centerX + radius, centerY}, color);
    writePixel((Point){centerX - radius, centerY}, color);
    
    // Draw circle using 8-way symmetry
    while (x < y) {
        if (decisionParam >= 0) {               // Move diagonally
            y--;
            deltaDecisionY += 2;
            decisionParam += deltaDecisionY;
        }
        x++;                                    // Always move right
        deltaDecisionX += 2;
        decisionParam += deltaDecisionX;
        
        // Draw 8 symmetric points for each calculated point
        writePixel((Point){centerX + x, centerY + y}, color);                // Octant 0
        writePixel((Point){centerX - x, centerY + y}, color);                // Octant 3
        writePixel((Point){centerX + x, centerY - y}, color);                // Octant 4
        writePixel((Point){centerX - x, centerY - y}, color);                // Octant 7
        writePixel((Point){centerX + y, centerY + x}, color);                // Octant 1
        writePixel((Point){centerX - y, centerY + x}, color);                // Octant 2
        writePixel((Point){centerX + y, centerY - x}, color);                // Octant 5
        writePixel((Point){centerX - y, centerY - x}, color);                // Octant 6
    }
}

/**
 * Draw a filled circle
 * Uses vertical line drawing to fill the circle efficiently
 */
static void writeFilledCircle(Point center, int16_t radius, OLED_color color) {
    int16_t centerX = center.x;
    int16_t centerY = center.y;
    
    // Draw center vertical line (with bounds checking)
    int16_t top = centerY - radius;
    int16_t bottom = centerY + radius;
    if (top < 0) top = 0;
    if (bottom >= HEIGHT) bottom = HEIGHT - 1;
    writeLine((Point){centerX, top}, (Point){centerX, bottom}, color);
    
    // Fill left and right halves using symmetry
    int16_t decisionParam = 1 - radius;
    int16_t deltaDecisionX = 1;
    int16_t deltaDecisionY = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;
    int16_t prevX = x;
    int16_t prevY = y;
    
    int16_t delta = 1;
    
    while (x < y) {
        if (decisionParam >= 0) {
            y--;
            deltaDecisionY += 2;
            decisionParam += deltaDecisionY;
        }
        x++;
        deltaDecisionX += 2;
        decisionParam += deltaDecisionX;
        
        // Calculate line endpoints with bounds checking
        int16_t lineTop = centerY - y;
        int16_t lineBottom = centerY + y + delta;
        if (lineTop < 0) lineTop = 0;
        if (lineBottom >= HEIGHT) lineBottom = HEIGHT - 1;
        
        // Draw vertical lines to fill, avoiding double-drawing
        if (x < (y + 1)) {
            if (centerX + x < WIDTH)
                writeLine((Point){centerX + x, lineTop}, (Point){centerX + x, lineBottom}, color);
            if (centerX - x >= 0)
                writeLine((Point){centerX - x, lineTop}, (Point){centerX - x, lineBottom}, color);
        }
        if (y != prevY) {
            int16_t prevLineTop = centerY - prevX;
            int16_t prevLineBottom = centerY + prevX + delta;
            if (prevLineTop < 0) prevLineTop = 0;
            if (prevLineBottom >= HEIGHT) prevLineBottom = HEIGHT - 1;
            
            if (centerX + prevY < WIDTH)
                writeLine((Point){centerX + prevY, prevLineTop}, (Point){centerX + prevY, prevLineBottom}, color);
            if (centerX - prevY >= 0)
                writeLine((Point){centerX - prevY, prevLineTop}, (Point){centerX - prevY, prevLineBottom}, color);
            prevY = y;
        }
        prevX = x;
    }
}

/**
 * Draw a quarter circle (for rounded rectangles - saved for future use)
 * @param center Center coordinates
 * @param radius Circle radius
 * @param cornerName Bitmap indicating which corner(s): bit 0=NW, 1=NE, 2=SE, 3=SW
 */
static void writeQuarterCircle(Point center, int16_t radius, uint8_t cornerName, OLED_color color) {
    int16_t centerX = center.x;
    int16_t centerY = center.y;
    
    int16_t decisionParam = 1 - radius;
    int16_t deltaDecisionX = 1;
    int16_t deltaDecisionY = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;
    
    while (x < y) {
        if (decisionParam >= 0) {
            y--;
            deltaDecisionY += 2;
            decisionParam += deltaDecisionY;
        }
        x++;
        deltaDecisionX += 2;
        decisionParam += deltaDecisionX;
        
        // Draw only the specified corner(s)
        if (cornerName & 0x4) {                                              // Southeast
            writePixel((Point){centerX + x, centerY + y}, color);
            writePixel((Point){centerX + y, centerY + x}, color);
        }
        if (cornerName & 0x2) {                                              // Northeast
            writePixel((Point){centerX + x, centerY - y}, color);
            writePixel((Point){centerX + y, centerY - x}, color);
        }
        if (cornerName & 0x8) {                                              // Southwest
            writePixel((Point){centerX - y, centerY + x}, color);
            writePixel((Point){centerX - x, centerY + y}, color);
        }
        if (cornerName & 0x1) {                                              // Northwest
            writePixel((Point){centerX - y, centerY - x}, color);
            writePixel((Point){centerX - x, centerY - y}, color);
        }
    }
}

/*============================================================================
 * RECTANGLE DRAWING PRIMITIVES
 *==========================================================================*/

/**
 * Draw a rectangle outline
 */
static void writeRect(Point topLeft, Point bottomRight, OLED_color color) {
    writeLine(topLeft, (Point){bottomRight.x, topLeft.y}, color);            // Top edge
    writeLine((Point){bottomRight.x, topLeft.y}, bottomRight, color);        // Right edge
    writeLine(bottomRight, (Point){topLeft.x, bottomRight.y}, color);        // Bottom edge
    writeLine((Point){topLeft.x, bottomRight.y}, topLeft, color);            // Left edge
}

/**
 * Draw a filled rectangle
 */
static void writeFilledRect(Point topLeft, Point bottomRight, OLED_color color) {
    int16_t x1 = topLeft.x, y1 = topLeft.y;
    int16_t x2 = bottomRight.x, y2 = bottomRight.y;
    
    // Normalize coordinates so x1,y1 is always top-left
    if (x2 < x1) {
        _swap_int16_t(x1, x2);
    }
    if (y2 < y1) {
        _swap_int16_t(y1, y2);
    }
    
    // Fill rectangle by drawing vertical lines
    for (; x1 <= x2; x1++) {
        writeLine((Point){x1, y1}, (Point){x1, y2}, color);
    }
}

/*============================================================================
 * SHAPE DRAW FUNCTIONS (Called via function pointers)
 *==========================================================================*/

/**
 * Internal draw function for circles
 * Reads the shape's is_filled attribute and calls appropriate function
 */
static void draw_circle(Shape* self, OLED_color color) {
    if (self == NULL || self->shape_data == NULL) return;
    
    CircleData* data = (CircleData*)self->shape_data;
    
    if (self->is_filled) {
        writeFilledCircle(data->center, data->radius, color);
    } else {
        writeCircle(data->center, data->radius, color);
    }
}

/**
 * Internal draw function for rectangles
 * Reads the shape's is_filled attribute and calls appropriate function
 */
static void draw_rectangle(Shape* self, OLED_color color) {
    if (self == NULL || self->shape_data == NULL) return;
    
    RectangleData* data = (RectangleData*)self->shape_data;
    
    if (self->is_filled) {
        writeFilledRect(data->top_left, data->bottom_right, color);
    } else {
        writeRect(data->top_left, data->bottom_right, color);
    }
}

/*============================================================================
 * SHAPE CONSTRUCTORS
 *==========================================================================*/

/**
 * Create a new circle shape
 */
Shape* create_circle(Point center, int16_t radius, uint8_t is_filled) {
    // Allocate memory for the shape structure
    Shape* shape = (Shape*)malloc(sizeof(Shape));
    if (shape == NULL) return NULL;
    
    // Allocate memory for circle-specific data
    CircleData* data = (CircleData*)malloc(sizeof(CircleData));
    if (data == NULL) {
        free(shape);
        return NULL;
    }
    
    // Initialize circle data
    data->center = center;
    data->radius = radius;
    
    // Initialize shape structure
    shape->draw = draw_circle;
    shape->shape_data = data;
    shape->type = SHAPE_CIRCLE;
    shape->is_filled = is_filled;
    
    return shape;
}

/**
 * Create a new rectangle shape
 */
Shape* create_rectangle(Point top_left, Point bottom_right, uint8_t is_filled) {
    // Allocate memory for the shape structure
    Shape* shape = (Shape*)malloc(sizeof(Shape));
    if (shape == NULL) return NULL;
    
    // Allocate memory for rectangle-specific data
    RectangleData* data = (RectangleData*)malloc(sizeof(RectangleData));
    if (data == NULL) {
        free(shape);
        return NULL;
    }
    
    // Initialize rectangle data
    data->top_left = top_left;
    data->bottom_right = bottom_right;
    
    // Initialize shape structure
    shape->draw = draw_rectangle;
    shape->shape_data = data;
    shape->type = SHAPE_RECTANGLE;
    shape->is_filled = is_filled;
    
    return shape;
}

/*============================================================================
 * SHAPE OPERATIONS (Polymorphic Interface)
 *==========================================================================*/

/**
 * Draw a shape using its polymorphic draw function
 */
void draw_shape(Shape* shape, OLED_color color) {
    if (shape != NULL && shape->draw != NULL) {
        shape->draw(shape, color);
    }
}

/**
 * Set the fill state of a shape
 */
void set_shape_isFilled(Shape* shape, uint8_t is_filled) {
    if (shape != NULL) {
        shape->is_filled = is_filled;
    }
}

/**
 * Toggle the fill state of a shape
 */
void toggle_shape_isFilled(Shape* shape) {
    if (shape != NULL) {
        shape->is_filled = (1-shape->is_filled);
    }
}

/**
 * Get the fill state of a shape
 */
uint8_t get_shape_isFilled(Shape* shape) {
    if (shape != NULL) {
        return shape->is_filled;
    }
    return 0;
}

/**
 * Get the type of a shape
 */
ShapeType get_shape_type(Shape* shape) {
    if (shape != NULL) {
        return shape->type;
    }
    return SHAPE_CIRCLE;  // Default return value
}

/**
 * Destroy a shape and free its memory
 */
void shape_destroy(Shape* shape) {
    if (shape != NULL) {
        if (shape->shape_data != NULL) {
            free(shape->shape_data);
        }
        free(shape);
    }
}

/*============================================================================
 * SHAPE MODIFICATION FUNCTIONS
 *==========================================================================*/

/**
 * Move a circle to a new position
 */
void set_circle_center(Shape* shape, Point new_center) {
    if (shape != NULL && shape->type == SHAPE_CIRCLE && shape->shape_data != NULL) {
        CircleData* data = (CircleData*)shape->shape_data;
        data->center = new_center;
    }
}

/**
 * Change a circle's radius
 */
void set_circle_radius(Shape* shape, int16_t new_radius) {
    if (shape != NULL && shape->type == SHAPE_CIRCLE && shape->shape_data != NULL) {
        CircleData* data = (CircleData*)shape->shape_data;
        data->radius = new_radius;
    }
}

/**
 * Move a rectangle to a new position
 */
void rectangle_set_position(Shape* shape, Point new_top_left, Point new_bottom_right) {
    if (shape != NULL && shape->type == SHAPE_RECTANGLE && shape->shape_data != NULL) {
        RectangleData* data = (RectangleData*)shape->shape_data;
        data->top_left = new_top_left;
        data->bottom_right = new_bottom_right;
    }
}