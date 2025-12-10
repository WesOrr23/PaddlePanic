/*============================================================================
 * physics.c - Created by Wes Orr (11/29/25)
 *============================================================================
 * Physics simulation layer for SH1106 graphics library
 * 
 * Implements movement, collision detection, and collision response
 *==========================================================================*/

#include "physics.h"
#include "shapes.h"
#include <stdlib.h>
#include <stddef.h>

/*============================================================================
 * HELPER MACROS
 *==========================================================================*/
#ifndef abs
#define abs(x) ((x) < 0 ? -(x) : (x))
#endif

/*============================================================================
 * PHYSICS OBJECT INITIALIZATION
 *==========================================================================*/

void init_physics(PhysicsObject* obj, Point position, Vector2D velocity,
                  ShapeType type, ShapeParams params,
                  CollisionCallback callback) {
    if (obj == NULL) return;

    Shape* shape = NULL;

    // Create shape based on type (ST7789 - always filled)
    if (type == SHAPE_CIRCLE) {
        shape = create_circle(position, params.circle.radius, params.circle.color);
    } else if (type == SHAPE_RECTANGLE) {
        shape = create_rectangle(position, params.rect.width, params.rect.height,
                               params.rect.anchor, params.rect.color);
    }

    if (shape == NULL) return;  // Handle allocation failure

    // Initialize physics object
    obj->position = position;
    obj->velocity = velocity;
    obj->acceleration.x = 0;
    obj->acceleration.y = 0;
    obj->visual = shape;
    obj->on_collision = callback;
    obj->collision_enabled = 1;
}

void destroy(PhysicsObject* obj) {
    if (obj != NULL && obj->visual != NULL) {
        destroy_shape(obj->visual);
        obj->visual = NULL;
    }
}

/*============================================================================
 * PHYSICS SIMULATION
 *==========================================================================*/

void move(PhysicsObject* obj, Vector2D delta) {
    if (obj == NULL) return;

    // Update position
    obj->position.x += delta.x;
    obj->position.y += delta.y;

    // Sync visual shape position
    if (obj->visual != NULL) {
        obj->visual->origin = obj->position;  // Direct access!
    }
}

void update(PhysicsObject* obj) {
    if (obj == NULL) return;
    
    // Apply velocity (move by velocity vector)
    move(obj, obj->velocity);
    
    // Note: Acceleration is not applied yet (reserved for future use)
    // When needed, add before move():
    // obj->velocity.x += obj->acceleration.x;
    // obj->velocity.y += obj->acceleration.y;
}

/*============================================================================
 * COLLISION DETECTION HELPERS
 *==========================================================================*/

/**
 * Check collision between two circles
 */
static uint8_t check_circle_circle_collision(PhysicsObject* objA, PhysicsObject* objB) {
    CircleData* circleA = (CircleData*)(objA->visual->shape_data);
    CircleData* circleB = (CircleData*)(objB->visual->shape_data);

    int16_t dx = objA->position.x - objB->position.x;
    int16_t dy = objA->position.y - objB->position.y;
    // Use int32_t to prevent overflow on 240×240 display
    // Max distance: 240² = 57,600 which overflows int16_t (max 32,767)
    int32_t distance_squared = (int32_t)dx * dx + (int32_t)dy * dy;
    int16_t radius_sum = circleA->radius + circleB->radius;
    int32_t radius_sum_squared = (int32_t)radius_sum * radius_sum;

    return (distance_squared <= radius_sum_squared);
}

/**
 * Check collision between circle and rectangle
 */
static uint8_t check_circle_rect_collision(PhysicsObject* circle_obj, PhysicsObject* rect_obj) {
    CircleData* circle = (CircleData*)(circle_obj->visual->shape_data);
    RectangleData* rect = (RectangleData*)(rect_obj->visual->shape_data);
    
    // Calculate actual rectangle corners based on anchor
    Point top_left, bottom_right;
    Point rect_origin = rect_obj->visual->origin;  // Direct access!
    
    switch (rect->anchor) {
        case ANCHOR_TOP_LEFT:
            top_left = rect_origin;
            bottom_right.x = rect_origin.x + rect->width;
            bottom_right.y = rect_origin.y + rect->height;
            break;
        case ANCHOR_BOTTOM_LEFT:
            top_left.x = rect_origin.x;
            top_left.y = rect_origin.y - rect->height;
            bottom_right.x = rect_origin.x + rect->width;
            bottom_right.y = rect_origin.y;
            break;
        case ANCHOR_CENTER:
            top_left.x = rect_origin.x - rect->width / 2;
            top_left.y = rect_origin.y - rect->height / 2;
            bottom_right.x = rect_origin.x + rect->width / 2;
            bottom_right.y = rect_origin.y + rect->height / 2;
            break;
    }
    
    // Find closest point on rectangle to circle center
    Point circle_center = circle_obj->visual->origin;  // Direct access!
    int16_t closest_x = circle_center.x;
    int16_t closest_y = circle_center.y;
    
    if (circle_center.x < top_left.x) closest_x = top_left.x;
    else if (circle_center.x > bottom_right.x) closest_x = bottom_right.x;
    
    if (circle_center.y < top_left.y) closest_y = top_left.y;
    else if (circle_center.y > bottom_right.y) closest_y = bottom_right.y;
    
    // Check if closest point is within circle radius
    int16_t dx = circle_center.x - closest_x;
    int16_t dy = circle_center.y - closest_y;
    // Use int32_t to prevent overflow on 240×240 display
    int32_t distance_squared = (int32_t)dx * dx + (int32_t)dy * dy;
    int32_t radius_squared = (int32_t)circle->radius * circle->radius;

    return (distance_squared <= radius_squared);
}

/**
 * Check collision between two rectangles
 */
static uint8_t check_rect_rect_collision(PhysicsObject* objA, PhysicsObject* objB) {
    RectangleData* rectA = (RectangleData*)(objA->visual->shape_data);
    RectangleData* rectB = (RectangleData*)(objB->visual->shape_data);
    
    Point origin1 = objA->visual->origin;
    Point origin2 = objB->visual->origin;
    
    // Calculate actual corners for both rectangles
    Point top_leftA, bottom_rightA, top_leftB, bottom_rightB;
    
    // Rectangle A
    switch (rectA->anchor) {
        case ANCHOR_TOP_LEFT:
            top_leftA = origin1;
            bottom_rightA.x = origin1.x + rectA->width;
            bottom_rightA.y = origin1.y + rectA->height;
            break;
        case ANCHOR_BOTTOM_LEFT:
            top_leftA.x = origin1.x;
            top_leftA.y = origin1.y - rectA->height;
            bottom_rightA.x = origin1.x + rectA->width;
            bottom_rightA.y = origin1.y;
            break;
        case ANCHOR_CENTER:
            top_leftA.x = origin1.x - rectA->width / 2;
            top_leftA.y = origin1.y - rectA->height / 2;
            bottom_rightA.x = origin1.x + rectA->width / 2;
            bottom_rightA.y = origin1.y + rectA->height / 2;
            break;
    }
    
    // Rectangle B
    switch (rectB->anchor) {
        case ANCHOR_TOP_LEFT:
            top_leftB = origin2;
            bottom_rightB.x = origin2.x + rectB->width;
            bottom_rightB.y = origin2.y + rectB->height;
            break;
        case ANCHOR_BOTTOM_LEFT:
            top_leftB.x = origin2.x;
            top_leftB.y = origin2.y - rectB->height;
            bottom_rightB.x = origin2.x + rectB->width;
            bottom_rightB.y = origin2.y;
            break;
        case ANCHOR_CENTER:
            top_leftB.x = origin2.x - rectB->width / 2;
            top_leftB.y = origin2.y - rectB->height / 2;
            bottom_rightB.x = origin2.x + rectB->width / 2;
            bottom_rightB.y = origin2.y + rectB->height / 2;
            break;
    }
    
    // AABB collision check
    return !(bottom_rightA.x < top_leftB.x || top_leftA.x > bottom_rightB.x ||
             bottom_rightA.y < top_leftB.y || top_leftA.y > bottom_rightB.y);
}

/*============================================================================
 * COLLISION DETECTION
 *==========================================================================*/

uint8_t check_collision(PhysicsObject* objA, PhysicsObject* objB) {
    if (objA == NULL || objB == NULL) return 0;
    if (objA->visual == NULL || objB->visual == NULL) return 0;
    
    // Check if collision detection is enabled for both objects
    if (!objA->collision_enabled || !objB->collision_enabled) return 0;
    
    uint8_t collision = 0;
    ShapeType a_type = get_shape_type(objA->visual);
    ShapeType b_type = get_shape_type(objB->visual);
    
    // Determine which collision check to use
    if (a_type == SHAPE_CIRCLE && b_type == SHAPE_CIRCLE) {
        collision = check_circle_circle_collision(objA, objB);
    }
    else if (a_type == SHAPE_CIRCLE && b_type == SHAPE_RECTANGLE) {
        collision = check_circle_rect_collision(objA, objB);
    }
    else if (a_type == SHAPE_RECTANGLE && b_type == SHAPE_CIRCLE) {
        collision = check_circle_rect_collision(objB, objA);
    }
    else if (a_type == SHAPE_RECTANGLE && b_type == SHAPE_RECTANGLE) {
        collision = check_rect_rect_collision(objA, objB);
    }
    
    // If collision detected, call both callbacks
    if (collision) {
        if (objA->on_collision != NULL) {
            objA->on_collision(objA, objB);
        }
        if (objB->on_collision != NULL) {
            objB->on_collision(objB, objA);
        }
    }

    return collision;
}

/*============================================================================
 * PROPERTY ACCESSORS
 *==========================================================================*/

void set_physics_position(PhysicsObject* obj, Point new_position) {
    if (obj != NULL) {
        obj->position = new_position;
        if (obj->visual != NULL) {
            obj->visual->origin = new_position;  // Direct access!
        }
    }
}

void set_physics_velocity(PhysicsObject* obj, Vector2D new_velocity) {
    if (obj != NULL) {
        obj->velocity = new_velocity;
    }
}

Point get_physics_position(PhysicsObject* obj) {
    if (obj != NULL) {
        return obj->position;
    }
    return (Point){0, 0};
}

Vector2D get_physics_velocity(PhysicsObject* obj) {
    if (obj != NULL) {
        return obj->velocity;
    }
    return (Vector2D){0, 0};
}

void set_physics_collision_enabled(PhysicsObject* obj, uint8_t enabled) {
    if (obj != NULL) {
        obj->collision_enabled = enabled;
    }
}

uint8_t get_physics_collision_enabled(PhysicsObject* obj) {
    if (obj != NULL) {
        return obj->collision_enabled;
    }
    return 0;
}

/*============================================================================
 * COMMON COLLISION BEHAVIORS
 *==========================================================================*/

void collision_bounce(PhysicsObject* self, PhysicsObject* other) {
    // If other is a rectangle, use its dimensions
    if (get_shape_type(other->visual) == SHAPE_RECTANGLE) {
        RectangleData* rect = (RectangleData*)(other->visual->shape_data);
        if (rect->height > rect->width) {
            // Tall rectangle = vertical wall = bounce X
            self->velocity.x = -self->velocity.x;
        } else {
            // Wide rectangle = horizontal wall = bounce Y
            self->velocity.y = -self->velocity.y;
        }
    } else {
        // Circle or unknown - use distance method
        int16_t dx = self->position.x - other->position.x;
        int16_t dy = self->position.y - other->position.y;
        if (abs(dx) > abs(dy)) {
            self->velocity.x = -self->velocity.x;
        } else {
            self->velocity.y = -self->velocity.y;
        }
    }
}

void collision_none(PhysicsObject* self, PhysicsObject* other) {
    // Do nothing - static object
    (void)self;   // Suppress unused parameter warning
    (void)other;
}