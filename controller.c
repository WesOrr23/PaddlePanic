/*============================================================================
 * controller.c - Created by Wes Orr (11/30/25)
 *============================================================================
 * High-level game controller implementation
 *==========================================================================*/

#include "controller.h"
#include <stddef.h>

/*============================================================================
 * HELPER FUNCTIONS
 *==========================================================================*/

/**
 * Normalize 10-bit ADC value (0-1023) to int8_t (-127 to +127)
 * Applies deadzone at controller level
 */
static int8_t normalize_analog(uint16_t raw_value) {
    const uint16_t CENTER = 512;

    // Calculate distance from center
    int16_t delta = (int16_t)raw_value - (int16_t)CENTER;

    // Apply deadzone
    if (delta < 0 && delta > -JOYSTICK_DEADZONE) {
        return 0;  // Within deadzone (negative side)
    } else if (delta >= 0 && delta < JOYSTICK_DEADZONE) {
        return 0;  // Within deadzone (positive side)
    }

    // Scale to int8_t range (-127 to +127)
    // Delta range: -512 to +511
    // Target range: -127 to +127
    // Scaling factor: 127/511 ≈ 1/4
    int16_t scaled = (delta * 127) / 511;

    // Clamp to int8_t range
    if (scaled > 127) scaled = 127;
    if (scaled < -127) scaled = -127;

    return (int8_t)scaled;
}

/*============================================================================
 * CONTROLLER INITIALIZATION
 *==========================================================================*/

void init_controller(Controller* ctrl) {
    if (ctrl == NULL) return;

    // Initialize ADC peripheral (must be called before creating analog devices)
    init_adc();

    // Create button devices
    // Both buttons are active-low with pull-up resistors
    // Callbacks set to NULL - using polling approach
    ctrl->button1 = create_button(&PORTC, PIN4_bm, 1, NULL, NULL);
    ctrl->button2 = create_button(&PORTC, PIN5_bm, 1, NULL, NULL);

    // Create analog devices (joystick axes)
    // Threshold set to ANALOG_THRESHOLD (10 = 1% of 1023 range)
    // Callbacks set to NULL - using polling approach
    ctrl->joystick_x = create_analog(0, ANALOG_THRESHOLD, NULL);  // ADC Channel 0 (PA0)
    ctrl->joystick_y = create_analog(1, ANALOG_THRESHOLD, NULL);  // ADC Channel 1 (PA1)

    // Initialize state to default values
    ctrl->button1_pressed = 0;
    ctrl->button2_pressed = 0;
    ctrl->joystick_x_value = 0;
    ctrl->joystick_y_value = 0;
}

void destroy_controller(Controller* ctrl) {
    if (ctrl == NULL) return;

    // Destroy all input devices
    destroy_input_device(ctrl->button1);
    destroy_input_device(ctrl->button2);
    destroy_input_device(ctrl->joystick_x);
    destroy_input_device(ctrl->joystick_y);
}

/*============================================================================
 * CONTROLLER UPDATE
 *==========================================================================*/

void update_controller(Controller* ctrl) {
    if (ctrl == NULL) return;

    // Poll all input devices (reads hardware)
    poll_input(ctrl->button1);
    poll_input(ctrl->button2);
    poll_input(ctrl->joystick_x);
    poll_input(ctrl->joystick_y);

    // Update button states (direct copy - already 0 or 1)
    ctrl->button1_pressed = (uint8_t)get_input_value(ctrl->button1);
    ctrl->button2_pressed = (uint8_t)get_input_value(ctrl->button2);

    // Update and normalize joystick values (apply deadzone)
    uint16_t raw_x = get_input_value(ctrl->joystick_x);
    uint16_t raw_y = get_input_value(ctrl->joystick_y);

    ctrl->joystick_x_value = normalize_analog(raw_x);
    ctrl->joystick_y_value = normalize_analog(raw_y);
}

/*============================================================================
 * ACCESSOR FUNCTIONS
 *==========================================================================*/

uint8_t controller_button1_pressed(Controller* ctrl) {
    return (ctrl != NULL) ? ctrl->button1_pressed : 0;
}

uint8_t controller_button2_pressed(Controller* ctrl) {
    return (ctrl != NULL) ? ctrl->button2_pressed : 0;
}

int8_t controller_joystick_x(Controller* ctrl) {
    return (ctrl != NULL) ? ctrl->joystick_x_value : 0;
}

int8_t controller_joystick_y(Controller* ctrl) {
    return (ctrl != NULL) ? ctrl->joystick_y_value : 0;
}
