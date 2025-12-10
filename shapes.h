/*============================================================================
 * shapes.h - Created by Wes Orr (11/29/25), adapted for ST7789 (12/9/25)
 *============================================================================
 * Object-oriented shape abstraction layer for ST7789 graphics library
 *
 * Provides polymorphic shape interface using function pointers for clean
 * separation between shape logic and low-level display operations.
 *
 * All shape drawing code (circles, rectangles) is implemented here,
 * built on top of the primitives in st7789_driver.h
 *
 * CHANGES FROM SH1106 VERSION:
 * - Uses ST7789 color display (grayscale 0-255)
 * - Removed is_filled parameter (always draws filled shapes)
 * - Updated for 240x240 display resolution
 *
 * USAGE:
 *     Shape* circle = create_circle((Point){120, 120}, 15, 255);
 *     draw(circle);
 *     set_shape_color(circle, 128);
 *     draw(circle);
 *     destroy_shape(circle);
 *==========================================================================*/

#ifndef SHAPES_H
#define SHAPES_H

#include <stdint.h>

/*============================================================================
 * POINT STRUCTURE
 *==========================================================================*/
typedef struct {
    int16_t x;
    int16_t y;
} Point;

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
    uint8_t color;                                  // Grayscale color (0-255)
} Shape;

/*============================================================================
 * SHAPE CONSTRUCTORS
 *==========================================================================*/

/**
 * Create a new circle shape (always filled)
 * @param origin Center coordinates
 * @param radius Circle radius in pixels
 * @param color Grayscale color (0=black, 255=white)
 * @return Pointer to new Shape, or NULL if allocation fails
 */
Shape* create_circle(Point origin, int16_t radius, uint8_t color);

/**
 * Create a new rectangle shape (always filled)
 * @param origin Origin point (meaning depends on anchor)
 * @param width Rectangle width in pixels
 * @param height Rectangle height in pixels
 * @param anchor Where origin is located (TOP_LEFT, BOTTOM_LEFT, CENTER)
 * @param color Grayscale color (0=black, 255=white)
 * @return Pointer to new Shape, or NULL if allocation fails
 */
Shape* create_rectangle(Point origin, int16_t width, int16_t height,
                       RectangleAnchor anchor, uint8_t color);

/*============================================================================
 * SHAPE OPERATIONS (Polymorphic Interface)
 *==========================================================================*/

/**
 * Draw a shape to the display
 * Calls the appropriate draw function based on shape type
 * Uses the shape's stored color
 * @param shape Pointer to shape to draw
 */
void draw(Shape* shape);

/**
 * Erase a shape by drawing it in black
 * Note: This doesn't restore what was behind the shape,
 * but clears the pixels to black. Will be improved with
 * differential rendering and layering in the future.
 * @param shape Pointer to shape to erase
 */
void erase(Shape* shape);

/**
 * Set the color of a shape
 * @param shape Pointer to shape to modify
 * @param color New grayscale color (0-255)
 */
void set_shape_color(Shape* shape, uint8_t color);

/**
 * Get the color of a shape
 * @param shape Pointer to shape
 * @return Current grayscale color
 */
uint8_t get_shape_color(Shape* shape);

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
void destroy_shape(Shape* shape);

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