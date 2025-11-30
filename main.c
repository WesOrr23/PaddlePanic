/*
 * main.c - Updated for new physics/shapes system
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 */
#include <xc.h>
#include <stddef.h>
#include "sh1106_graphics.h"
#include "shapes.h"
#include "physics.h"

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 63

/*============================================================================
 * COLLISION CALLBACKS
 *==========================================================================*/

void ball_bounce(PhysicsObject* self, PhysicsObject* other) {
    // Bounce and toggle fill
    collision_bounce(self, other);
    toggle_shape_filled(self->visual);
}

void wall_hit(PhysicsObject* self, PhysicsObject* other) {
    // Do nothing - walls are static
    collision_none(self, other);
}

/*============================================================================
 * MAIN PROGRAM
 *==========================================================================*/

int main(void) {
    initSPI();
    initScreen();
    
    // Create boundary walls
    PhysicsObject walls[4];
    Shape* wall_top = create_rectangle((Point){0, 0}, 128, 3, ANCHOR_TOP_LEFT, 1, COLOR_WHITE);
    Shape* wall_bottom = create_rectangle((Point){0, HEIGHT-1}, 128, 3, ANCHOR_BOTTOM_LEFT, 1, COLOR_WHITE);
    Shape* wall_left = create_rectangle((Point){0, 0}, 3, 64, ANCHOR_TOP_LEFT, 1, COLOR_WHITE);
    Shape* wall_right = create_rectangle((Point){WIDTH-4, 0}, 3, 64, ANCHOR_TOP_LEFT, 1, COLOR_WHITE);
    
    init_physics(&walls[0], (Point){0, 0}, (Vector2D){0, 0}, wall_top, wall_hit);
    init_physics(&walls[1], (Point){0, HEIGHT-1}, (Vector2D){0, 0}, wall_bottom, wall_hit);
    init_physics(&walls[2], (Point){0, 0}, (Vector2D){0, 0}, wall_left, wall_hit);
    init_physics(&walls[3], (Point){WIDTH-4, 0}, (Vector2D){0, 0}, wall_right, wall_hit);
    
    // Create ball
    PhysicsObject ball;
    Shape* ball_shape = create_circle((Point){64, 32}, 5, 1, COLOR_WHITE);
    init_physics(&ball, (Point){64, 32}, (Vector2D){3, 2}, ball_shape, ball_bounce);
    
    // Main game loop
    while (1) {
        // Update physics
        update(&ball);
        
        // Check collisions with all walls
        for (int i = 0; i < 4; i++) {
            check_collision(&ball, &walls[i]);
        }
        
        // Draw everything
        clearDisplay();
        
        // Draw walls
        for (int i = 0; i < 4; i++) {
            draw(walls[i].visual);
        }
        
        // Draw ball
        draw(ball.visual);
        
        refreshDisplay();
    }
    
    return 0;
}