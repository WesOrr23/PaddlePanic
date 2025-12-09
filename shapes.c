/*============================================================================
 * shapes.c - Created by Wes Orr (11/29/25), adapted for ST7789 (12/9/25)
 *============================================================================
 * Object-oriented shape abstraction layer for ST7789 graphics library
 *
 * Implements polymorphic shape interface and all shape-specific drawing
 * code (circles, rectangles) using primitives from st7789_driver.c
 *
 * CHANGES FROM SH1106 VERSION:
 * - Uses ST7789 color display with built-in circle and rectangle functions
 * - Removed is_filled parameter (always draws filled shapes)
 * - Simplified drawing using st7789_drawCircle and direct pixel streaming
 *==========================================================================*/

#include "shapes.h"
#include "st7789_driver.h"
#include <stdlib.h>
#include <stddef.h>

/*============================================================================
 * INTERNAL MACROS
 *==========================================================================*/
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

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

        default:
            // Fallback to center anchor behavior if invalid anchor value
            temp_tl_x = origin.x - width / 2;
            temp_tl_y = origin.y - height / 2;
            temp_br_x = origin.x + width / 2;
            temp_br_y = origin.y + height / 2;
            break;
    }

    // Clamp to screen bounds
    tl->x = (temp_tl_x < 0) ? 0 : ((temp_tl_x >= ST7789_WIDTH) ? ST7789_WIDTH - 1 : temp_tl_x);
    tl->y = (temp_tl_y < 0) ? 0 : ((temp_tl_y >= ST7789_HEIGHT) ? ST7789_HEIGHT - 1 : temp_tl_y);
    br->x = (temp_br_x < 0) ? 0 : ((temp_br_x >= ST7789_WIDTH) ? ST7789_WIDTH - 1 : temp_br_x);
    br->y = (temp_br_y < 0) ? 0 : ((temp_br_y >= ST7789_HEIGHT) ? ST7789_HEIGHT - 1 : temp_br_y);
}

/**
 * Draw filled rectangle using ST7789 primitives
 */
static void drawFilledRect(Point origin, int16_t width, int16_t height,
                          RectangleAnchor anchor, uint8_t gray) {
    Point tl, br;
    calculate_rect_corners(origin, width, height, anchor, &tl, &br);

    int16_t x1 = tl.x, y1 = tl.y;
    int16_t x2 = br.x, y2 = br.y;

    if (x2 < x1) _swap_int16_t(x1, x2);
    if (y2 < y1) _swap_int16_t(y1, y2);

    // Draw rectangle row by row
    int16_t rect_width = x2 - x1;
    int16_t rect_height = y2 - y1;

    if (rect_width <= 0 || rect_height <= 0) return;

    ST7789_Color color = st7789_grayscale(gray);

    for (int16_t row = y1; row < y2; row++) {
        st7789_setAddrWindow(x1, row, rect_width, 1);
        st7789_beginData();
        for (int16_t col = 0; col < rect_width; col++) {
            st7789_write16(color);
        }
        st7789_endData();
    }
}

/*============================================================================
 * SHAPE DRAW FUNCTIONS
 *==========================================================================*/

static void draw_circle(Shape* self) {
    if (self == NULL || self->shape_data == NULL) return;

    CircleData* data = (CircleData*)self->shape_data;

    // Always draw filled circle
    st7789_drawCircle(self->origin.x, self->origin.y, data->radius,
                     st7789_grayscale(self->color), 1);
}

static void draw_rectangle(Shape* self) {
    if (self == NULL || self->shape_data == NULL) return;

    RectangleData* data = (RectangleData*)self->shape_data;

    // Always draw filled rectangle
    drawFilledRect(self->origin, data->width, data->height, data->anchor, self->color);
}

/*============================================================================
 * SHAPE CONSTRUCTORS
 *==========================================================================*/

Shape* create_circle(Point origin, int16_t radius, uint8_t color) {
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
    shape->color = color;

    return shape;
}

Shape* create_rectangle(Point origin, int16_t width, int16_t height,
                       RectangleAnchor anchor, uint8_t color) {
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

void set_shape_color(Shape* shape, uint8_t color) {
    if (shape != NULL) {
        shape->color = color;
    }
}

uint8_t get_shape_color(Shape* shape) {
    return (shape != NULL) ? shape->color : 0;
}

ShapeType get_shape_type(Shape* shape) {
    return (shape != NULL) ? shape->type : SHAPE_CIRCLE;
}

void destroy_shape(Shape* shape) {
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
