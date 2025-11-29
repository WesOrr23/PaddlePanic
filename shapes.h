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
 *     Shape* circle = create_circle((Point){64, 32}, 15, 1);
 *     shape_draw(circle, COLOR_WHITE);
 *     shape_set_filled(circle, 0);
 *     shape_draw(circle, COLOR_WHITE);
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
 * SHAPE DATA STRUCTURES
 *==========================================================================*/

/**
 * Circle-specific data
 */
typedef struct {
    Point center;
    int16_t radius;
} CircleData;

/**
 * Rectangle-specific data
 */
typedef struct {
    Point top_left;
    Point bottom_right;
} RectangleData;

/**
 * Base Shape structure (polymorphic interface)
 * All shapes contain this structure for uniform handling
 */
typedef struct Shape {
    void (*draw)(struct Shape* self, OLED_color color);    // Polymorphic draw function
    void* shape_data;                                       // Pointer to specific shape data
    ShapeType type;                                         // Shape type identifier
    uint8_t is_filled;                                      // Fill state
} Shape;

/*============================================================================
 * SHAPE CONSTRUCTORS
 *==========================================================================*/

/**
 * Create a new circle shape
 * @param center Center coordinates
 * @param radius Circle radius in pixels
 * @param is_filled 1 for filled circle, 0 for outline
 * @return Pointer to new Shape, or NULL if allocation fails
 */
Shape* create_circle(Point center, int16_t radius, uint8_t is_filled);

/**
 * Create a new rectangle shape
 * @param top_left Top-left corner coordinates
 * @param bottom_right Bottom-right corner coordinates
 * @param is_filled 1 for filled rectangle, 0 for outline
 * @return Pointer to new Shape, or NULL if allocation fails
 */
Shape* create_rectangle(Point top_left, Point bottom_right, uint8_t is_filled);

/*============================================================================
 * SHAPE OPERATIONS (Polymorphic Interface)
 *==========================================================================*/

/**
 * Draw a shape to the display buffer
 * Calls the appropriate draw function based on shape type
 * @param shape Pointer to shape to draw
 * @param color COLOR_WHITE, COLOR_BLACK, or COLOR_INVERT
 */
void draw_shape(Shape* shape, OLED_color color);

/**
 * Set the fill state of a shape
 * @param shape Pointer to shape to modify
 * @param is_filled 1 for filled, 0 for outline
 */
void set_shape_isFilled(Shape* shape, uint8_t is_filled);

/**
 * Toggle the fill state of a shape
 * @param shape Pointer to shape to modify
 */
void toggle_shape_isFilled(Shape* shape);

/**
 * Get the fill state of a shape
 * @param shape Pointer to shape
 * @return 1 if filled, 0 if outline
 */
uint8_t get_shape_isFilled(Shape* shape);

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
 * Move a circle to a new position
 * @param shape Pointer to circle shape
 * @param new_center New center coordinates
 */
void set_circle_center(Shape* shape, Point new_center);

/**
 * Change a circle's radius
 * @param shape Pointer to circle shape
 * @param new_radius New radius in pixels
 */
void set_circle_radius(Shape* shape, int16_t new_radius);

/**
 * Move a rectangle to a new position
 * @param shape Pointer to rectangle shape
 * @param new_top_left New top-left corner coordinates
 * @param new_bottom_right New bottom-right corner coordinates
 */
void rectangle_set_position(Shape* shape, Point new_top_left, Point new_bottom_right);

#endif // SHAPES_H