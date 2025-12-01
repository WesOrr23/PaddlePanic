/*============================================================================
 * controller.h - Created by Wes Orr (11/30/25)
 *============================================================================
 * High-level game controller abstraction
 *
 * Owns and manages InputDevices (buttons, joystick axes)
 * Maps hardware inputs to game-ready values
 * Applies deadzone at controller level (game logic concern)
 *
 * Architecture:
 *     main.c (game logic)
 *         ↓
 *     controller.c (input mapping, normalization)
 *         ↓
 *     io_hardware.c (hardware abstraction)
 *         ↓
 *     ATtiny1627 GPIO/ADC peripherals
 *
 * USAGE:
 *     // Create controller (initializes all input devices)
 *     Controller ctrl;
 *     init_controller(&ctrl);
 *
 *     // In game loop
 *     update_controller(&ctrl);
 *
 *     // Read inputs
 *     if (controller_button1_pressed(&ctrl)) {
 *         // Button 1 pressed
 *     }
 *
 *     int8_t joy_y = controller_joystick_y(&ctrl);  // -127 to +127
 *     paddle.velocity.y = joy_y / 16;  // Scale for game speed
 *
 *     // Cleanup
 *     destroy_controller(&ctrl);
 *==========================================================================*/

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include "io_hardware.h"

/*============================================================================
 * CONFIGURATION MACROS
 *==========================================================================*/

/**
 * Joystick deadzone radius (applied at controller layer)
 * Values within this range of center (512) are treated as zero
 * Prevents jitter when joystick is at rest
 */
#define JOYSTICK_DEADZONE 50

/*============================================================================
 * CONTROLLER STRUCTURE
 *==========================================================================*/

/**
 * Controller structure
 * Owns all input devices and provides game-ready input values
 */
typedef struct {
    // Input devices (owned by controller)
    InputDevice* button1;       // Onboard button (PC4)
    InputDevice* button2;       // Joystick button (PC5)
    InputDevice* joystick_x;    // Joystick X axis (ADC channel 0)
    InputDevice* joystick_y;    // Joystick Y axis (ADC channel 1)

    // Processed input state (game-ready values)
    uint8_t button1_pressed;    // 1 if pressed, 0 if released
    uint8_t button2_pressed;    // 1 if pressed, 0 if released
    int8_t joystick_x_value;    // Normalized: -127 to +127 (0 at center, deadzone applied)
    int8_t joystick_y_value;    // Normalized: -127 to +127 (0 at center, deadzone applied)
} Controller;

/*============================================================================
 * CONTROLLER INITIALIZATION
 *==========================================================================*/

/**
 * Initialize controller and create all input devices
 *
 * Hardware configuration:
 * - Button 1: PORTC PIN4 (PC4) - Onboard button, active-low with pull-up
 * - Button 2: PORTC PIN5 (PC5) - Joystick button, active-low with pull-up
 * - Joystick X: ADC Channel 1 (PORTA PIN1/AIN1)
 * - Joystick Y: ADC Channel 2 (PORTA PIN2/AIN2)
 *
 * Initializes ADC peripheral and creates all InputDevices with:
 * - Threshold: 10 (1% of 1023 range) for analog inputs
 * - Callbacks: NULL (polling-based, not using callbacks)
 *
 * @param ctrl Pointer to controller to initialize
 */
void init_controller(Controller* ctrl);

/**
 * Destroy controller and free all input devices
 * @param ctrl Pointer to controller to destroy
 */
void destroy_controller(Controller* ctrl);

/*============================================================================
 * CONTROLLER OPERATIONS
 *==========================================================================*/

/**
 * Update controller state (poll all input devices)
 * Call once per frame in game loop
 *
 * This function:
 * - Polls all input devices (reads hardware)
 * - Normalizes analog values to int8_t range
 * - Applies deadzone to joystick inputs
 * - Updates controller state variables
 *
 * @param ctrl Pointer to controller
 */
void update_controller(Controller* ctrl);

/**
 * Get button 1 state
 * @param ctrl Pointer to controller
 * @return 1 if pressed, 0 if released
 */
uint8_t controller_button1_pressed(Controller* ctrl);

/**
 * Get button 2 state
 * @param ctrl Pointer to controller
 * @return 1 if pressed, 0 if released
 */
uint8_t controller_button2_pressed(Controller* ctrl);

/**
 * Get joystick X axis value (normalized with deadzone)
 * @param ctrl Pointer to controller
 * @return Normalized value: -127 (left) to +127 (right), 0 at center
 */
int8_t controller_joystick_x(Controller* ctrl);

/**
 * Get joystick Y axis value (normalized with deadzone)
 * @param ctrl Pointer to controller
 * @return Normalized value: -127 (up) to +127 (down), 0 at center
 */
int8_t controller_joystick_y(Controller* ctrl);

#endif // CONTROLLER_H
