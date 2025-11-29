/*
 * main.c
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 */

#include <xc.h>
#include "sh1106_graphics.h"

#define SCREEN_WIDTH  127
#define SCREEN_HEIGHT 63

typedef struct {
    uint8_t radius;
    Point position;                                                          // Center position of circle
    int8_t dx;                                                               // X velocity
    int8_t dy;                                                               // Y velocity
} Circle;

void updateCircle(Circle *c) {
    c->position.x += c->dx;
    c->position.y += c->dy;
    
    // Bounce off left/right edges
    if (c->position.x >= SCREEN_WIDTH - c->radius || c->position.x < c->radius) {
        c->dx = -c->dx;
    }
    
    // Bounce off top/bottom edges
    if (c->position.y >= SCREEN_HEIGHT - c->radius || c->position.y < c->radius) {
        c->dy = -c->dy;
    }
}

int main(void) {
    initSPI();
    initScreen();
    
    Circle circles[] = {
        {10, {10, 10}, 1, 2},
        {5,  {5, 5},   2, 3},
        {2,  {2, 2},   1, 5}
    };
    
    while (1) {
        clearDisplay();  // Clear entire buffer first
        
        // Update positions FIRST
        for (int i = 0; i < 3; i++) {
            updateCircle(&circles[i]);
        }
        
        // Then draw all circles
        for (int i = 0; i < 3; i++) {
            writeCircle(circles[i].position, circles[i].radius, COLOR_WHITE);
        }
        
        showScreen();  // Update display once
    }
}