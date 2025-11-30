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

static void writeCircle(Point center, int16_t radius, OLED_color color) {
    int16_t centerX = center.x;
    int16_t centerY = center.y;
    
    int16_t decisionParam = 1 - radius;
    int16_t deltaDecisionX = 1;
    int16_t deltaDecisionY = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;
    
    writePixel((Point){centerX, centerY + radius}, color);
    writePixel((Point){centerX, centerY - radius}, color);
    writePixel((Point){centerX + radius, centerY}, color);
    writePixel((Point){centerX - radius, centerY}, color);
    
    while (x < y) {
        if (decisionParam >= 0) {
            y--;
            deltaDecisionY += 2;
            decisionParam += deltaDecisionY;
        }
        x++;
        deltaDecisionX += 2;
        decisionParam += deltaDecisionX;
        
        writePixel((Point){centerX + x, centerY + y}, color);
        writePixel((Point){centerX - x, centerY + y}, color);
        writePixel((Point){centerX + x, centerY - y}, color);
        writePixel((Point){centerX - x, centerY - y}, color);
        writePixel((Point){centerX + y, centerY + x}, color);
        writePixel((Point){centerX - y, centerY + x}, color);
        writePixel((Point){centerX + y, centerY - x}, color);
        writePixel((Point){centerX - y, centerY - x}, color);
    }
}

static void writeFilledCircle(Point center, int16_t radius, OLED_color color) {
    int16_t centerX = center.x;
    int16_t centerY = center.y;
    
    int16_t top = centerY - radius;
    int16_t bottom = centerY + radius;
    if (top < 0) top = 0;
    if (bottom >= HEIGHT) bottom = HEIGHT - 1;
    writeLine((Point){centerX, top}, (Point){centerX, bottom}, color);
    
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
        
        int16_t lineTop = centerY - y;
        int16_t lineBottom = centerY + y + delta;
        if (lineTop < 0) lineTop = 0;
        if (lineBottom >= HEIGHT) lineBottom = HEIGHT - 1;
        
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

/*============================================================================
 * RECTANGLE DRAWING PRIMITIVES
 *==========================================================================*/

/**
 * Calculate actual top-left and bottom-right corners based on anchor
 * Corners are clamped to screen bounds for safe drawing
 */
static void calculate_rect_corners(Point origin, int16_t width, int16_t height,
                                   RectangleAnchor anchor, Point* tl, Point* br) {
    int16_t temp_tl_x, temp_tl_y, temp_br_x, temp_br_y;
    
    switch (anchor) {
        case ANCHOR_TOP_LEFT:
            temp_tl_x = origin.x;
            temp_tl_y = origin.y;
            temp_br_x = origin.x + width;
            temp_br_y = origin.y + height;
            break;
            
        case ANCHOR_BOTTOM_LEFT:
            temp_tl_x = origin.x;
            temp_tl_y = origin.y - height;
            temp_br_x = origin.x + width;
            temp_br_y = origin.y;
            break;
            
        case ANCHOR_CENTER:
            temp_tl_x = origin.x - width / 2;
            temp_tl_y = origin.y - height / 2;
            temp_br_x = origin.x + width / 2;
            temp_br_y = origin.y + height / 2;
            break;
    }
    
    // Clamp to screen bounds
    // Note: We don't prevent negative values, just clamp when assigning to uint8_t
    tl->x = (temp_tl_x < 0) ? 0 : ((temp_tl_x >= WIDTH) ? WIDTH - 1 : temp_tl_x);
    tl->y = (temp_tl_y < 0) ? 0 : ((temp_tl_y >= HEIGHT) ? HEIGHT - 1 : temp_tl_y);
    br->x = (temp_br_x < 0) ? 0 : ((temp_br_x >= WIDTH) ? WIDTH - 1 : temp_br_x);
    br->y = (temp_br_y < 0) ? 0 : ((temp_br_y >= HEIGHT) ? HEIGHT - 1 : temp_br_y);
}

static void writeRect(Point origin, int16_t width, int16_t height, 
                     RectangleAnchor anchor, OLED_color color) {
    Point tl, br;
    calculate_rect_corners(origin, width, height, anchor, &tl, &br);
    
    // Draw outline - br is exclusive, so subtract 1 for actual edge
    writeLine(tl, (Point){br.x - 1, tl.y}, color);           // Top edge
    writeLine((Point){br.x - 1, tl.y}, (Point){br.x - 1, br.y - 1}, color);  // Right edge
    writeLine((Point){br.x - 1, br.y - 1}, (Point){tl.x, br.y - 1}, color);  // Bottom edge
    writeLine((Point){tl.x, br.y - 1}, tl, color);           // Left edge
}

static void writeFilledRect(Point origin, int16_t width, int16_t height,
                           RectangleAnchor anchor, OLED_color color) {
    Point tl, br;
    calculate_rect_corners(origin, width, height, anchor, &tl, &br);
    
    int16_t x1 = tl.x, y1 = tl.y;
    int16_t x2 = br.x, y2 = br.y;
    
    if (x2 < x1) _swap_int16_t(x1, x2);
    if (y2 < y1) _swap_int16_t(y1, y2);
    
    // Fill rectangle - br is exclusive, so use < instead of <=
    for (; x1 < x2; x1++) {
        writeLine((Point){x1, y1}, (Point){x1, y2 - 1}, color);
    }
}

/*============================================================================
 * SHAPE DRAW FUNCTIONS
 *==========================================================================*/

static void draw_circle(Shape* self) {
    if (self == NULL || self->shape_data == NULL) return;
    
    CircleData* data = (CircleData*)self->shape_data;
    
    if (self->is_filled) {
        writeFilledCircle(self->origin, data->radius, self->color);
    } else {
        writeCircle(self->origin, data->radius, self->color);
    }
}

static void draw_rectangle(Shape* self) {
    if (self == NULL || self->shape_data == NULL) return;
    
    RectangleData* data = (RectangleData*)self->shape_data;
    
    if (self->is_filled) {
        writeFilledRect(self->origin, data->width, data->height, data->anchor, self->color);
    } else {
        writeRect(self->origin, data->width, data->height, data->anchor, self->color);
    }
}

/*============================================================================
 * SHAPE CONSTRUCTORS
 *==========================================================================*/

Shape* create_circle(Point origin, int16_t radius, uint8_t is_filled, OLED_color color) {
    Shape* shape = (Shape*)malloc(sizeof(Shape));
    if (shape == NULL) return NULL;
    
    CircleData* data = (CircleData*)malloc(sizeof(CircleData));
    if (data == NULL) {
        free(shape);
        return NULL;
    }
    
    data->radius = radius;
    
    shape->origin = origin;
    shape->draw_impl = draw_circle;
    shape->shape_data = data;
    shape->type = SHAPE_CIRCLE;
    shape->is_filled = is_filled;
    shape->color = color;
    
    return shape;
}

Shape* create_rectangle(Point origin, int16_t width, int16_t height,
                       RectangleAnchor anchor, uint8_t is_filled, OLED_color color) {
    Shape* shape = (Shape*)malloc(sizeof(Shape));
    if (shape == NULL) return NULL;
    
    RectangleData* data = (RectangleData*)malloc(sizeof(RectangleData));
    if (data == NULL) {
        free(shape);
        return NULL;
    }
    
    data->anchor = anchor;
    data->width = width;
    data->height = height;
    
    shape->origin = origin;
    shape->draw_impl = draw_rectangle;
    shape->shape_data = data;
    shape->type = SHAPE_RECTANGLE;
    shape->is_filled = is_filled;
    shape->color = color;
    
    return shape;
}

/*============================================================================
 * SHAPE OPERATIONS
 *==========================================================================*/

void draw(Shape* shape) {
    if (shape != NULL && shape->draw_impl != NULL) {
        shape->draw_impl(shape);
    }
}

void set_shape_filled(Shape* shape, uint8_t is_filled) {
    if (shape != NULL) {
        shape->is_filled = is_filled;
    }
}

void toggle_shape_filled(Shape* shape) {
    if (shape != NULL) {
        shape->is_filled = !shape->is_filled;
    }
}

uint8_t get_shape_filled(Shape* shape) {
    return (shape != NULL) ? shape->is_filled : 0;
}

void set_shape_color(Shape* shape, OLED_color color) {
    if (shape != NULL) {
        shape->color = color;
    }
}

OLED_color get_shape_color(Shape* shape) {
    return (shape != NULL) ? shape->color : COLOR_BLACK;
}

ShapeType get_shape_type(Shape* shape) {
    return (shape != NULL) ? shape->type : SHAPE_CIRCLE;
}

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

void set_shape_position(Shape* shape, Point new_origin) {
    if (shape != NULL) {
        shape->origin = new_origin;
    }
}

Point get_shape_position(Shape* shape) {
    if (shape != NULL) {
        return shape->origin;
    }
    return (Point){0, 0};
}

void set_circle_radius(Shape* shape, int16_t new_radius) {
    if (shape != NULL && shape->type == SHAPE_CIRCLE && shape->shape_data != NULL) {
        CircleData* data = (CircleData*)shape->shape_data;
        data->radius = new_radius;
    }
}

int16_t get_circle_radius(Shape* shape) {
    if (shape != NULL && shape->type == SHAPE_CIRCLE && shape->shape_data != NULL) {
        CircleData* data = (CircleData*)shape->shape_data;
        return data->radius;
    }
    return 0;
}

void set_rectangle_dimensions(Shape* shape, int16_t new_width, int16_t new_height) {
    if (shape != NULL && shape->type == SHAPE_RECTANGLE && shape->shape_data != NULL) {
        RectangleData* data = (RectangleData*)shape->shape_data;
        data->width = new_width;
        data->height = new_height;
    }
}

void set_rectangle_anchor(Shape* shape, RectangleAnchor new_anchor) {
    if (shape != NULL && shape->type == SHAPE_RECTANGLE && shape->shape_data != NULL) {
        RectangleData* data = (RectangleData*)shape->shape_data;
        data->anchor = new_anchor;
    }
}