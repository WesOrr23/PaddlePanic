/*
 * main.c
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 */
#include <xc.h>
#include <stddef.h>  // For NULL definition
#include "sh1106_graphics.h"
#include "shapes.h"

#define SCREEN_WIDTH  127
#define SCREEN_HEIGHT 63

typedef struct {
    uint8_t radius;
    Point position;                                                          // Center position of circle
    int8_t dx;                                                               // X velocity
    int8_t dy;                                                               // Y velocity
    Shape* shape;                                                            // Pointer to the shape object
} Circle;

void updateCircle(Circle *c) {
    c->position.x += c->dx;
    c->position.y += c->dy;
    
    // Bounce off left/right edges
    if (c->position.x >= SCREEN_WIDTH - c->radius || c->position.x < c->radius) {
        c->dx = -c->dx;
        set_shape_isFilled(c->shape, 1-(get_shape_isFilled(c->shape)));
    }
    
    // Bounce off top/bottom edges
    if (c->position.y >= SCREEN_HEIGHT - c->radius || c->position.y < c->radius) {
        c->dy = -c->dy;
        set_shape_isFilled(c->shape, 1-(get_shape_isFilled(c->shape)));
    }
    
    // Update the shape's position
    set_circle_center(c->shape, c->position);
}

int main(void) {
    initSPI();
    initScreen();
    
    // Define circle data
    Circle circles[] = {
        {10, {10, 10}, 1, 2, NULL},
        {5,  {50, 30}, 2, 3, NULL},
        {2,  {100, 50}, 1, 5, NULL}
    };
    
    // Create shape objects for each circle
    for (int i = 0; i < 3; i++) {
        circles[i].shape = create_circle(circles[i].position, circles[i].radius, 1);  // 1 = filled
    }
    
    while (1) {
        clearDisplay();  // Clear entire buffer first
        
        // Update positions FIRST
        for (int i = 0; i < 3; i++) {
            updateCircle(&circles[i]);
        }
        
        // Then draw all circles
        for (int i = 0; i < 3; i++) {
            draw_shape(circles[i].shape, COLOR_WHITE);
        }
        
        showScreen();  // Update display once
    }
    
    // Clean up (never reached in this infinite loop, but good practice)
    for (int i = 0; i < 3; i++) {
        shape_destroy(circles[i].shape);
    }
}