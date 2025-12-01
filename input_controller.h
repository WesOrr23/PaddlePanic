/*============================================================================
 * input_controller.h - Created by Wes Orr (11/30/25)
 *============================================================================
 * Hardware input controller abstraction
 *
 * Manages InputDevices (buttons, joystick axes) and reports raw hardware
 * values to game controller layer. No normalization or game logic.
 *
 * Architecture:
 *     game_controller.c (game logic, normalization)
 *         ↓
 *     input_controller.c (hardware management)
 *         ↓
 *     io_hardware.c (low-level devices)
 *         ↓
 *     ATtiny1627 GPIO/ADC peripherals
 *
 * USAGE:
 *     // Create input controller (initializes all input devices)
 *     InputController ctrl;
 *     init_input_controller(&ctrl);
 *
 *     // In game loop
 *     update_input_controller(&ctrl);
 *
 *     // Read raw inputs
 *     if (input_controller_button1_pressed(&ctrl)) {
 *         // Button 1 pressed
 *     }
 *
 *     uint16_t raw_x = input_controller_joystick_x(&ctrl);  // 0-4095
 *
 *     // Cleanup
 *     destroy_input_controller(&ctrl);
 *==========================================================================*/

#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <stdint.h>
#include "io_hardware.h"

/*============================================================================
 * INPUT CONTROLLER STRUCTURE
 *==========================================================================*/

/**
 * InputController structure
 * Owns all input devices and provides raw hardware values
 */
typedef struct {
    // Input devices (owned by input controller)
    InputDevice* button1;       // Onboard button (PC4)
    InputDevice* button2;       // Joystick button (PC5)
    InputDevice* joystick_x;    // Joystick X axis (ADC channel 1)
    InputDevice* joystick_y;    // Joystick Y axis (ADC channel 2)

    // Raw hardware state (no processing)
    uint8_t button1_pressed;    // 1 if pressed, 0 if released
    uint8_t button2_pressed;    // 1 if pressed, 0 if released
    uint16_t joystick_x_raw;    // Raw ADC: 0-4095
    uint16_t joystick_y_raw;    // Raw ADC: 0-4095
} InputController;

/*============================================================================
 * INPUT CONTROLLER INITIALIZATION
 *==========================================================================*/

/**
 * Initialize input controller and create all input devices
 *
 * Hardware configuration:
 * - Button 1: PORTC PIN4 (PC4) - Onboard button, active-low with pull-up
 * - Button 2: PORTC PIN5 (PC5) - Joystick button, active-low with pull-up
 * - Joystick X: ADC Channel 1 (PORTA PIN1/AIN1)
 * - Joystick Y: ADC Channel 2 (PORTA PIN2/AIN2)
 *
 * Initializes ADC peripheral and creates all InputDevices with:
 * - Threshold: 10 for analog inputs (hardware filtering only)
 * - Callbacks: NULL (polling-based, not using callbacks)
 *
 * @param ctrl Pointer to input controller to initialize
 */
void init_input_controller(InputController* ctrl);

/**
 * Destroy input controller and free all input devices
 * @param ctrl Pointer to input controller to destroy
 */
void destroy_input_controller(InputController* ctrl);

/*============================================================================
 * INPUT CONTROLLER OPERATIONS
 *==========================================================================*/

/**
 * Update input controller state (poll all input devices)
 * Call once per frame in game loop
 *
 * This function:
 * - Polls all input devices (reads hardware)
 * - Stores raw values (no normalization or deadzone)
 *
 * @param ctrl Pointer to input controller
 */
void update_input_controller(InputController* ctrl);

/**
 * Get button 1 state
 * @param ctrl Pointer to input controller
 * @return 1 if pressed, 0 if released
 */
uint8_t input_controller_button1_pressed(InputController* ctrl);

/**
 * Get button 2 state
 * @param ctrl Pointer to input controller
 * @return 1 if pressed, 0 if released
 */
uint8_t input_controller_button2_pressed(InputController* ctrl);

/**
 * Get joystick X axis raw value
 * @param ctrl Pointer to input controller
 * @return Raw ADC value: 0-4095
 */
uint16_t input_controller_joystick_x(InputController* ctrl);

/**
 * Get joystick Y axis raw value
 * @param ctrl Pointer to input controller
 * @return Raw ADC value: 0-4095
 */
uint16_t input_controller_joystick_y(InputController* ctrl);

#endif // INPUT_CONTROLLER_H
