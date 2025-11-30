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

void init_physics(PhysicsObject* obj, Point pos, Vector2D vel, 
                  Shape* shape, CollisionCallback callback) {
    if (obj == NULL) return;
    
    obj->position = pos;
    obj->velocity = vel;
    obj->acceleration.x = 0;
    obj->acceleration.y = 0;
    obj->visual = shape;
    obj->on_collision = callback;
    obj->collision_enabled = 1;
    
    // Sync visual shape position with physics position
    if (shape != NULL) {
        shape->origin = pos;  // Direct access - much simpler!
    }
}

void destroy_physics(PhysicsObject* obj) {
    if (obj != NULL && obj->visual != NULL) {
        shape_destroy(obj->visual);
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

void update_physics(PhysicsObject* obj) {
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
static uint8_t check_circle_circle_collision(PhysicsObject* obj1, PhysicsObject* obj2) {
    CircleData* c1 = (CircleData*)(obj1->visual->shape_data);
    CircleData* c2 = (CircleData*)(obj2->visual->shape_data);
    
    int16_t dx = obj1->position.x - obj2->position.x;
    int16_t dy = obj1->position.y - obj2->position.y;
    int16_t distance_squared = dx * dx + dy * dy;
    int16_t radius_sum = c1->radius + c2->radius;
    int16_t radius_sum_squared = radius_sum * radius_sum;
    
    return (distance_squared <= radius_sum_squared);
}

/**
 * Check collision between circle and rectangle
 */
static uint8_t check_circle_rect_collision(PhysicsObject* circle_obj, PhysicsObject* rect_obj) {
    CircleData* c = (CircleData*)(circle_obj->visual->shape_data);
    RectangleData* r = (RectangleData*)(rect_obj->visual->shape_data);
    
    // Calculate actual rectangle corners based on anchor
    Point tl, br;
    Point rect_origin = rect_obj->visual->origin;  // Direct access!
    
    switch (r->anchor) {
        case ANCHOR_TOP_LEFT:
            tl = rect_origin;
            br.x = rect_origin.x + r->width;
            br.y = rect_origin.y + r->height;
            break;
        case ANCHOR_BOTTOM_LEFT:
            tl.x = rect_origin.x;
            tl.y = rect_origin.y - r->height;
            br.x = rect_origin.x + r->width;
            br.y = rect_origin.y;
            break;
        case ANCHOR_CENTER:
            tl.x = rect_origin.x - r->width / 2;
            tl.y = rect_origin.y - r->height / 2;
            br.x = rect_origin.x + r->width / 2;
            br.y = rect_origin.y + r->height / 2;
            break;
    }
    
    // Find closest point on rectangle to circle center
    Point circle_center = circle_obj->visual->origin;  // Direct access!
    int16_t closest_x = circle_center.x;
    int16_t closest_y = circle_center.y;
    
    if (circle_center.x < tl.x) closest_x = tl.x;
    else if (circle_center.x > br.x) closest_x = br.x;
    
    if (circle_center.y < tl.y) closest_y = tl.y;
    else if (circle_center.y > br.y) closest_y = br.y;
    
    // Check if closest point is within circle radius
    int16_t dx = circle_center.x - closest_x;
    int16_t dy = circle_center.y - closest_y;
    int16_t distance_squared = dx * dx + dy * dy;
    
    return (distance_squared <= (c->radius * c->radius));
}

/**
 * Check collision between two rectangles
 */
static uint8_t check_rect_rect_collision(PhysicsObject* obj1, PhysicsObject* obj2) {
    RectangleData* r1 = (RectangleData*)(obj1->visual->shape_data);
    RectangleData* r2 = (RectangleData*)(obj2->visual->shape_data);
    
    Point origin1 = obj1->visual->origin;  // Direct access!
    Point origin2 = obj2->visual->origin;  // Direct access!
    
    // Calculate actual corners for both rectangles
    Point tl1, br1, tl2, br2;
    
    // Rectangle 1
    switch (r1->anchor) {
        case ANCHOR_TOP_LEFT:
            tl1 = origin1;
            br1.x = origin1.x + r1->width;
            br1.y = origin1.y + r1->height;
            break;
        case ANCHOR_BOTTOM_LEFT:
            tl1.x = origin1.x;
            tl1.y = origin1.y - r1->height;
            br1.x = origin1.x + r1->width;
            br1.y = origin1.y;
            break;
        case ANCHOR_CENTER:
            tl1.x = origin1.x - r1->width / 2;
            tl1.y = origin1.y - r1->height / 2;
            br1.x = origin1.x + r1->width / 2;
            br1.y = origin1.y + r1->height / 2;
            break;
    }
    
    // Rectangle 2
    switch (r2->anchor) {
        case ANCHOR_TOP_LEFT:
            tl2 = origin2;
            br2.x = origin2.x + r2->width;
            br2.y = origin2.y + r2->height;
            break;
        case ANCHOR_BOTTOM_LEFT:
            tl2.x = origin2.x;
            tl2.y = origin2.y - r2->height;
            br2.x = origin2.x + r2->width;
            br2.y = origin2.y;
            break;
        case ANCHOR_CENTER:
            tl2.x = origin2.x - r2->width / 2;
            tl2.y = origin2.y - r2->height / 2;
            br2.x = origin2.x + r2->width / 2;
            br2.y = origin2.y + r2->height / 2;
            break;
    }
    
    // AABB collision check
    return !(br1.x < tl2.x || tl1.x > br2.x ||
             br1.y < tl2.y || tl1.y > br2.y);
}

/*============================================================================
 * COLLISION DETECTION
 *==========================================================================*/

uint8_t check_physics_collision_objects(PhysicsObject* obj1, PhysicsObject* obj2) {
    if (obj1 == NULL || obj2 == NULL) return 0;
    if (obj1->visual == NULL || obj2->visual == NULL) return 0;
    
    // Check if collision detection is enabled for both objects
    if (!obj1->collision_enabled || !obj2->collision_enabled) return 0;
    
    uint8_t collision = 0;
    ShapeType type1 = get_shape_type(obj1->visual);
    ShapeType type2 = get_shape_type(obj2->visual);
    
    // Determine which collision check to use
    if (type1 == SHAPE_CIRCLE && type2 == SHAPE_CIRCLE) {
        collision = check_circle_circle_collision(obj1, obj2);
    }
    else if (type1 == SHAPE_CIRCLE && type2 == SHAPE_RECTANGLE) {
        collision = check_circle_rect_collision(obj1, obj2);
    }
    else if (type1 == SHAPE_RECTANGLE && type2 == SHAPE_CIRCLE) {
        collision = check_circle_rect_collision(obj2, obj1);
    }
    else if (type1 == SHAPE_RECTANGLE && type2 == SHAPE_RECTANGLE) {
        collision = check_rect_rect_collision(obj1, obj2);
    }
    
    // If collision detected, call both callbacks
    if (collision) {
        if (obj1->on_collision != NULL) {
            obj1->on_collision(obj1, obj2);
        }
        if (obj2->on_collision != NULL) {
            obj2->on_collision(obj2, obj1);
        }
    }
    
    return collision;
}

/*============================================================================
 * PROPERTY ACCESSORS
 *==========================================================================*/

void set_physics_position(PhysicsObject* obj, Point new_pos) {
    if (obj != NULL) {
        obj->position = new_pos;
        if (obj->visual != NULL) {
            obj->visual->origin = new_pos;  // Direct access!
        }
    }
}

void set_physics_velocity(PhysicsObject* obj, Vector2D new_vel) {
    if (obj != NULL) {
        obj->velocity = new_vel;
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