/*
 * main.c - Input System Testing (Temporary Button Test)
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 */
#include <xc.h>
#include <stddef.h>
#include "sh1106_graphics.h"
#include "shapes.h"
#include "physics.h"
#include "controller.h"

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

    // ========== FULL CONTROLLER SYSTEM TEST ==========
    // Tests entire input system: io_hardware + controller layers

    // Initialize controller (creates all input devices, initializes ADC)
    Controller ctrl;
    init_controller(&ctrl);

    // Create controllable paddle
    const uint8_t PADDLE_WIDTH = 5;
    const uint8_t PADDLE_HEIGHT = 20;
    Point paddle_pos = {10, 32};  // Start on left side, vertically centered

    // Calculate clamping bounds based on paddle dimensions and ANCHOR_CENTER
    // For ANCHOR_CENTER, origin is at center of shape
    const uint8_t half_width = PADDLE_WIDTH / 2;
    const uint8_t half_height = PADDLE_HEIGHT / 2;
    const uint8_t min_x = half_width;
    const uint8_t max_x = SCREEN_WIDTH - 1 - half_width;
    const uint8_t min_y = half_height;
    const uint8_t max_y = SCREEN_HEIGHT - 1 - half_height;

    while (1) {
        // Poll controller (reads all hardware, applies normalization & deadzone)
        update_controller(&ctrl);

        // Get normalized joystick values (-127 to +127, with deadzone)
        int8_t joy_x = controller_joystick_x(&ctrl);
        int8_t joy_y = controller_joystick_y(&ctrl);

        // Update paddle position based on joystick
        // Use int16_t to avoid uint8_t underflow when adding negative values
        int16_t new_x = paddle_pos.x + (joy_x / 8);
        int16_t new_y = paddle_pos.y + (joy_y / 8);

        // Clamp to screen bounds (accounting for paddle size and anchor)
        if (new_x < min_x) new_x = min_x;
        if (new_x > max_x) new_x = max_x;
        if (new_y < min_y) new_y = min_y;
        if (new_y > max_y) new_y = max_y;

        // Assign back to position
        paddle_pos.x = (uint8_t)new_x;
        paddle_pos.y = (uint8_t)new_y;

        // Clear display
        clearDisplay();

        // Draw paddle (vertical bar on left side)
        Shape* paddle = create_rectangle(paddle_pos, PADDLE_WIDTH, PADDLE_HEIGHT, ANCHOR_CENTER, 1, COLOR_WHITE);
        draw(paddle);
        destroy_shape(paddle);

        // Draw center reference cross
        Shape* center_h = create_rectangle((Point){64, 32}, 8, 1, ANCHOR_CENTER, 1, COLOR_WHITE);
        Shape* center_v = create_rectangle((Point){64, 32}, 1, 8, ANCHOR_CENTER, 1, COLOR_WHITE);
        draw(center_h);
        draw(center_v);
        destroy_shape(center_h);
        destroy_shape(center_v);

        // Draw button indicators (top corners)
        // Button 1 (top-left corner) - onboard button
        if (controller_button1_pressed(&ctrl)) {
            Shape* btn1 = create_rectangle((Point){5, 5}, 8, 8, ANCHOR_TOP_LEFT, 1, COLOR_WHITE);
            draw(btn1);
            destroy_shape(btn1);
        } else {
            Shape* btn1 = create_rectangle((Point){5, 5}, 8, 8, ANCHOR_TOP_LEFT, 0, COLOR_WHITE);
            draw(btn1);
            destroy_shape(btn1);
        }

        // Button 2 (top-right corner) - joystick button
        if (controller_button2_pressed(&ctrl)) {
            Shape* btn2 = create_rectangle((Point){115, 5}, 8, 8, ANCHOR_TOP_LEFT, 1, COLOR_WHITE);
            draw(btn2);
            destroy_shape(btn2);
        } else {
            Shape* btn2 = create_rectangle((Point){115, 5}, 8, 8, ANCHOR_TOP_LEFT, 0, COLOR_WHITE);
            draw(btn2);
            destroy_shape(btn2);
        }

        refreshDisplay();
    }

    // Cleanup (never reached in this test)
    destroy_controller(&ctrl);
    return 0;
}