/*============================================================================
 * input_controller.c - Created by Wes Orr (11/30/25)
 *============================================================================
 * Hardware input controller implementation
 *==========================================================================*/

#include "input_controller.h"
#include <stddef.h>

/*============================================================================
 * INPUT CONTROLLER INITIALIZATION
 *==========================================================================*/

void init_input_controller(InputController* ctrl) {
    if (ctrl == NULL) return;

    // Initialize ADC peripheral (must be called before creating analog devices)
    init_adc();

    // Create button devices
    // Button 1 (onboard): active-low with pull-up
    // Button 2 (joystick): active-low with pull-up
    // Callbacks set to NULL - using polling approach
    ctrl->button1 = create_button(&PORTC, PIN4_bm, 1, NULL, NULL);  // Active-low
    ctrl->button2 = create_button(&PORTC, PIN5_bm, 1, NULL, NULL);  // Active-low

    // Create analog devices (joystick axes)
    // Threshold set to ANALOG_THRESHOLD (10 for hardware filtering)
    // Callbacks set to NULL - using polling approach
    ctrl->joystick_x = create_analog(1, ANALOG_THRESHOLD, NULL);  // ADC Channel 1 (PA1)
    ctrl->joystick_y = create_analog(2, ANALOG_THRESHOLD, NULL);  // ADC Channel 2 (PA2)

    // Initialize state to default values
    ctrl->button1_pressed = 0;
    ctrl->button2_pressed = 0;
    ctrl->joystick_x_raw = 2048;  // Center
    ctrl->joystick_y_raw = 2048;  // Center
}

void destroy_input_controller(InputController* ctrl) {
    if (ctrl == NULL) return;

    // Destroy all input devices
    destroy_input_device(ctrl->button1);
    destroy_input_device(ctrl->button2);
    destroy_input_device(ctrl->joystick_x);
    destroy_input_device(ctrl->joystick_y);
}

/*============================================================================
 * INPUT CONTROLLER UPDATE
 *==========================================================================*/

void update_input_controller(InputController* ctrl) {
    if (ctrl == NULL) return;

    // Poll all input devices (reads hardware)
    poll_input(ctrl->button1);
    poll_input(ctrl->button2);
    poll_input(ctrl->joystick_x);
    poll_input(ctrl->joystick_y);

    // Update button states (direct copy - already 0 or 1)
    ctrl->button1_pressed = (uint8_t)get_input_value(ctrl->button1);
    ctrl->button2_pressed = (uint8_t)get_input_value(ctrl->button2);

    // Store raw joystick values (no processing)
    ctrl->joystick_x_raw = get_input_value(ctrl->joystick_x);
    ctrl->joystick_y_raw = get_input_value(ctrl->joystick_y);
}

/*============================================================================
 * ACCESSOR FUNCTIONS
 *==========================================================================*/

uint8_t input_controller_button1_pressed(InputController* ctrl) {
    return (ctrl != NULL) ? ctrl->button1_pressed : 0;
}

uint8_t input_controller_button2_pressed(InputController* ctrl) {
    return (ctrl != NULL) ? ctrl->button2_pressed : 0;
}

uint16_t input_controller_joystick_x(InputController* ctrl) {
    return (ctrl != NULL) ? ctrl->joystick_x_raw : 2048;
}

uint16_t input_controller_joystick_y(InputController* ctrl) {
    return (ctrl != NULL) ? ctrl->joystick_y_raw : 2048;
}
