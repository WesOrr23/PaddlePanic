/*
 * main.c
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 */

#include <xc.h>
#include "ACU_SH1106_gfxlib.h"

#define SCREEN_WIDTH 127
#define SCREEN_HEIGHT 63

typedef struct {
    uint8_t radius;
    uint8_t x;
    uint8_t y;
    int8_t dx;
    int8_t dy;
} Circle;

void updateCircle(Circle *c) {
    c->x += c->dx;
    c->y += c->dy;

    if (c->x >= SCREEN_WIDTH - c->radius || c->x < c->radius) {
        c->dx = -c->dx;
    }
    if (c->y >= SCREEN_HEIGHT - c->radius || c->y < c->radius) {
        c->dy = -c->dy;
    }
}

int main(void) {
    initSPI();
    initScreen();

    Circle circles[] = {
        {10, 10, 10, 1, 2},
        {5, 5, 5, 2, 3},
        {2, 2, 2, 1, 5}
    };

    while (1) {
        // Draw all circles
        for (int i = 0; i < 3; i++) {
            writeCircle(circles[i].x, circles[i].y, circles[i].radius, COLOR_WHITE);
        }

        showScreen();

        // Erase all circles
        for (int i = 0; i < 3; i++) {
            writeCircle(circles[i].x, circles[i].y, circles[i].radius, COLOR_BLACK);
        }

        // Update positions
        for (int i = 0; i < 3; i++) {
            updateCircle(&circles[i]);
        }
    }
}