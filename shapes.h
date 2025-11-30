/*============================================================================
 * shapes.h - Created by Wes Orr (11/29/25)
 *============================================================================
 * Object-oriented shape abstraction layer for SH1106 graphics library
 * 
 * Provides polymorphic shape interface using function pointers for clean
 * separation between shape logic and low-level display operations.
 *
 * All shape drawing code (circles, rectangles) is implemented here,
 * built on top of the primitives in sh1106_graphics.h (pixels, lines).
 *
 * USAGE:
 *     Shape* circle = create_circle((Point){64, 32}, 15, 1, COLOR_WHITE);
 *     draw(circle);
 *     set_shape_filled(circle, 0);
 *     draw(circle);
 *     shape_destroy(circle);
 *==========================================================================*/

#ifndef SHAPES_H
#define SHAPES_H

#include <stdint.h>
#include "sh1106_graphics.h"

/*============================================================================
 * SHAPE TYPE ENUMERATION
 *==========================================================================*/
typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECTANGLE
} ShapeType;

/*============================================================================
 * RECTANGLE ANCHOR ENUMERATION
 *==========================================================================*/
typedef enum {
    ANCHOR_TOP_LEFT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_CENTER
} RectangleAnchor;

/*============================================================================
 * SHAPE DATA STRUCTURES
 *==========================================================================*/

/**
 * Circle-specific data
 * Origin (center) is stored in parent Shape
 */
typedef struct {
    int16_t radius;
} CircleData;

/**
 * Rectangle-specific data
 * Origin location depends on anchor setting, stored in parent Shape
 */
typedef struct {
    RectangleAnchor anchor;
    int16_t width;
    int16_t height;
} RectangleData;

/**
 * Base Shape structure (polymorphic interface)
 * All shapes contain this structure for uniform handling
 */
typedef struct Shape {
    Point origin;                                   // Position of shape (meaning depends on type/anchor)
    void (*draw_impl)(struct Shape* self);          // Polymorphic draw implementation
    void* shape_data;                               // Pointer to specific shape data
    ShapeType type;                                 // Shape type identifier
    uint8_t is_filled;                              // Fill state
    OLED_color color;                               // Color to draw shape
} Shape;

/*============================================================================
 * SHAPE CONSTRUCTORS
 *==========================================================================*/

/**
 * Create a new circle shape
 * @param origin Center coordinates
 * @param radius Circle radius in pixels
 * @param is_filled 1 for filled circle, 0 for outline
 * @param color Color to draw the circle
 * @return Pointer to new Shape, or NULL if allocation fails
 */
Shape* create_circle(Point origin, int16_t radius, uint8_t is_filled, OLED_color color);

/**
 * Create a new rectangle shape
 * @param origin Origin point (meaning depends on anchor)
 * @param width Rectangle width in pixels
 * @param height Rectangle height in pixels
 * @param anchor Where origin is located (TOP_LEFT, BOTTOM_LEFT, CENTER)
 * @param is_filled 1 for filled rectangle, 0 for outline
 * @param color Color to draw the rectangle
 * @return Pointer to new Shape, or NULL if allocation fails
 */
Shape* create_rectangle(Point origin, int16_t width, int16_t height, 
                       RectangleAnchor anchor, uint8_t is_filled, OLED_color color);

/*============================================================================
 * SHAPE OPERATIONS (Polymorphic Interface)
 *==========================================================================*/

/**
 * Draw a shape to the display buffer
 * Calls the appropriate draw function based on shape type
 * Uses the shape's stored color
 * @param shape Pointer to shape to draw
 */
void draw(Shape* shape);

/**
 * Set the fill state of a shape
 * @param shape Pointer to shape to modify
 * @param is_filled 1 for filled, 0 for outline
 */
void set_shape_filled(Shape* shape, uint8_t is_filled);

/**
 * Toggle the fill state of a shape
 * @param shape Pointer to shape to modify
 */
void toggle_shape_filled(Shape* shape);

/**
 * Get the fill state of a shape
 * @param shape Pointer to shape
 * @return 1 if filled, 0 if outline
 */
uint8_t get_shape_filled(Shape* shape);

/**
 * Set the color of a shape
 * @param shape Pointer to shape to modify
 * @param color New color
 */
void set_shape_color(Shape* shape, OLED_color color);

/**
 * Get the color of a shape
 * @param shape Pointer to shape
 * @return Current color
 */
OLED_color get_shape_color(Shape* shape);

/**
 * Get the type of a shape
 * @param shape Pointer to shape
 * @return ShapeType enumeration value
 */
ShapeType get_shape_type(Shape* shape);

/**
 * Destroy a shape and free its memory
 * Always call this when done with a shape to prevent memory leaks
 * @param shape Pointer to shape to destroy
 */
void shape_destroy(Shape* shape);

/*============================================================================
 * SHAPE MODIFICATION FUNCTIONS
 *==========================================================================*/

/**
 * Set shape origin (works for all shape types)
 * For circles: origin is center
 * For rectangles: origin meaning depends on anchor
 * @param shape Pointer to shape
 * @param new_origin New origin position
 */
void set_shape_position(Shape* shape, Point new_origin);

/**
 * Get shape origin (works for all shape types)
 * @param shape Pointer to shape
 * @return Current origin
 */
Point get_shape_position(Shape* shape);

/**
 * Change a circle's radius
 * @param shape Pointer to circle shape
 * @param new_radius New radius in pixels
 */
void set_circle_radius(Shape* shape, int16_t new_radius);

/**
 * Get a circle's radius
 * @param shape Pointer to circle shape
 * @return Current radius, or 0 if not a circle
 */
int16_t get_circle_radius(Shape* shape);

/**
 * Change a rectangle's dimensions
 * @param shape Pointer to rectangle shape
 * @param new_width New width in pixels
 * @param new_height New height in pixels
 */
void set_rectangle_dimensions(Shape* shape, int16_t new_width, int16_t new_height);

/**
 * Change a rectangle's anchor point
 * @param shape Pointer to rectangle shape
 * @param new_anchor New anchor setting
 */
void set_rectangle_anchor(Shape* shape, RectangleAnchor new_anchor);

#endif // SHAPES_H