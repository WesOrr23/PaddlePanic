/*============================================================================
 * physics.h - Created by Wes Orr (11/29/25)
 *============================================================================
 * Physics simulation layer for SH1106 graphics library
 * 
 * Provides object-oriented physics interface with collision detection and
 * customizable collision response via callbacks.
 *
 * Architecture:
 *     main.c (game logic)
 *         ↓
 *     physics.c (movement, collision)
 *         ↓
 *     shapes.c (visual representation)
 *         ↓
 *     sh1106_graphics.c (display primitives)
 *
 * USAGE:
 *     // Define collision behavior
 *     void bounce(PhysicsObject* self, PhysicsObject* other) {
 *         collision_bounce(self, other);
 *     }
 *
 *     // Create physics object (physics now creates shape internally)
 *     PhysicsObject ball;
 *     init_physics(&ball, (Point){64, 32}, (Vector2D){2, 3},
 *                  SHAPE_CIRCLE, (ShapeParams){.circle = {5, 1, COLOR_WHITE}},
 *                  bounce);
 *
 *     // In game loop
 *     update(&ball);  // Applies velocity
 *     check_collision(&ball, &paddle);
 *
 *     // Draw (call shape draw directly)
 *     clearDisplay();
 *     draw(ball.visual);
 *     draw(paddle.visual);
 *     refreshDisplay();
 *==========================================================================*/

#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include "shapes.h"

/*============================================================================
 * TYPE DEFINITIONS
 *==========================================================================*/

/**
 * 2D Vector for velocity and acceleration
 * Using int8_t for integer-based physics (signed for negative values)
 */
typedef struct {
    int8_t x;
    int8_t y;
} Vector2D;

/**
 * Circle shape parameters for physics object creation
 * Updated for ST7789 - always filled, grayscale color
 */
typedef struct {
    int16_t radius;
    uint8_t color;  // Grayscale 0-255
} CircleParams;

/**
 * Rectangle shape parameters for physics object creation
 * Updated for ST7789 - always filled, grayscale color
 */
typedef struct {
    int16_t width;
    int16_t height;
    RectangleAnchor anchor;
    uint8_t color;  // Grayscale 0-255
} RectangleParams;

/**
 * Union holding either circle or rectangle parameters
 * Use designated initializers to specify which type:
 *   (ShapeParams){.circle = {5, 255}}        // Circle: radius 5, white
 *   (ShapeParams){.rect = {10, 20, ANCHOR_CENTER, 255}}  // Rectangle: 10×20, white
 */
typedef union {
    CircleParams circle;
    RectangleParams rect;
} ShapeParams;

// Forward declaration for callback type
typedef struct PhysicsObject PhysicsObject;

/**
 * Collision callback function type
 * Called when this object collides with another object
 * @param self Pointer to this physics object
 * @param other Pointer to the object this collided with
 */
typedef void (*CollisionCallback)(PhysicsObject* self, PhysicsObject* other);

/**
 * Physics Object structure
 * Combines position, velocity, acceleration with visual representation
 */
struct PhysicsObject {
    Point position;                 // Current position (screen coordinates)
    Point prev_position;            // Previous position (for differential rendering)
    Vector2D velocity;              // Velocity in pixels per frame
    Vector2D acceleration;          // Acceleration (reserved for future use)
    Shape* visual;                  // Visual representation (shape to draw)
    CollisionCallback on_collision; // Callback function when collision occurs
    uint8_t collision_enabled;      // 1 = check collisions, 0 = ignore
};

/*============================================================================
 * PHYSICS OBJECT INITIALIZATION
 *==========================================================================*/

/**
 * Initialize a physics object with internal shape creation
 * Physics object now owns and creates its visual shape
 * @param obj Pointer to physics object to initialize
 * @param position Initial position (screen coordinates)
 * @param velocity Initial velocity
 * @param type Shape type (SHAPE_CIRCLE or SHAPE_RECTANGLE)
 * @param params Shape parameters (use designated initializers for union)
 * @param callback Function to call on collision (can be NULL for no response)
 *
 * Example usage:
 *   init_physics(&ball, (Point){64, 32}, (Vector2D){2, 3},
 *                SHAPE_CIRCLE, (ShapeParams){.circle = {5, 1, COLOR_WHITE}},
 *                ball_bounce);
 */
void init_physics(PhysicsObject* obj, Point position, Vector2D velocity,
                  ShapeType type, ShapeParams params,
                  CollisionCallback callback);

/**
 * Destroy a physics object (frees internal shape)
 * The PhysicsObject itself is not freed (assumed static allocation)
 * @param obj Pointer to physics object to destroy
 */
void destroy(PhysicsObject* obj);

/*============================================================================
 * PHYSICS SIMULATION
 *==========================================================================*/

/**
 * Move physics object by a given delta
 * This is a general movement function that can be used for:
 * - Automatic physics updates (delta = velocity)
 * - Manual player control (delta = input)
 * @param obj Pointer to physics object to move
 * @param delta Movement vector to apply
 */
void move(PhysicsObject* obj, Vector2D delta);

/**
 * Update physics object (applies velocity to position)
 * Convenience function that calls move() with the object's velocity
 * @param obj Pointer to physics object to update
 */
void update(PhysicsObject* obj);

/*============================================================================
 * COLLISION DETECTION
 *==========================================================================*/

/**
 * Check collision between two physics objects
 * Uses bounding box collision based on shape type
 * If collision detected AND both objects have collision_enabled:
 *   - Calls both objects' collision callbacks
 * @param obj1 First physics object
 * @param obj2 Second physics object
 * @return 1 if collision detected, 0 otherwise
 */
uint8_t check_collision(PhysicsObject* objA, PhysicsObject* objB);

/*============================================================================
 * PROPERTY ACCESSORS
 *==========================================================================*/

/**
 * Set the position of a physics object
 * Also updates the visual shape's position
 * @param obj Pointer to physics object
 * @param new_pos New position
 */
void set_physics_position(PhysicsObject* obj, Point new_position);

/**
 * Set the velocity of a physics object
 * @param obj Pointer to physics object
 * @param new_vel New velocity
 */
void set_physics_velocity(PhysicsObject* obj, Vector2D new_velocity);

/**
 * Get the current position of a physics object
 * @param obj Pointer to physics object
 * @return Current position
 */
Point get_physics_position(PhysicsObject* obj);

/**
 * Get the current velocity of a physics object
 * @param obj Pointer to physics object
 * @return Current velocity
 */
Vector2D get_physics_velocity(PhysicsObject* obj);

/**
 * Enable or disable collision detection for an object
 * @param obj Pointer to physics object
 * @param enabled 1 to enable, 0 to disable
 */
void set_physics_collision_enabled(PhysicsObject* obj, uint8_t enabled);

/**
 * Check if collision detection is enabled for an object
 * @param obj Pointer to physics object
 * @return 1 if enabled, 0 if disabled
 */
uint8_t get_physics_collision_enabled(PhysicsObject* obj);

/*============================================================================
 * COMMON COLLISION BEHAVIORS (Pre-defined callbacks)
 *==========================================================================*/

/**
 * Bounce behavior - reverses velocity component based on collision direction
 * This is a common callback you can use for bouncing objects
 * @param self The object that is bouncing
 * @param other The object it collided with
 */
void collision_bounce(PhysicsObject* self, PhysicsObject* other);

/**
 * Do nothing behavior - no response to collision
 * Useful for static objects like walls that don't need to respond
 * @param self The object (unused)
 * @param other The other object (unused)
 */
void collision_none(PhysicsObject* self, PhysicsObject* other);

#endif // PHYSICS_H