/*============================================================================
 * io_hardware.h - Created by Wes Orr (11/30/25)
 *============================================================================
 * Low-level hardware input abstraction for ATtiny1627
 *
 * Provides polymorphic InputDevice interface for digital buttons (GPIO)
 * and analog joystick axes (ADC with 10-bit resolution).
 *
 * Features:
 * - Callback-based event system (on_press, on_release, on_value_change)
 * - Internal relative threshold logic for analog inputs
 * - Polling-based (no interrupts)
 * - Exposes single "accepted value" per device
 *
 * Architecture:
 *     controller.c (game-level input mapping)
 *         ↓
 *     io_hardware.c (hardware abstraction)
 *         ↓
 *     ATtiny1627 GPIO/ADC peripherals
 *
 * USAGE:
 *     // Initialize ADC peripheral
 *     init_adc();
 *
 *     // Create button (active-low with pull-up)
 *     InputDevice* btn = create_button(&PORTC, PIN4_bm, 1, NULL, NULL);
 *
 *     // Create analog input (joystick axis)
 *     InputDevice* joy_x = create_analog(0, 10, NULL);  // Channel 0, threshold 10
 *
 *     // In game loop
 *     poll_input(btn);
 *     poll_input(joy_x);
 *
 *     uint16_t btn_state = get_input_value(btn);     // 0 or 1
 *     uint16_t joy_x_val = get_input_value(joy_x);   // 0-1023
 *
 *     // Cleanup
 *     destroy_input_device(btn);
 *     destroy_input_device(joy_x);
 *==========================================================================*/

#ifndef IO_HARDWARE_H
#define IO_HARDWARE_H

#include <stdint.h>
#include <xc.h>

/*============================================================================
 * CONFIGURATION MACROS
 *==========================================================================*/

/**
 * Analog input threshold - minimum change to trigger callback
 * Can be tuned for sensitivity (lower = more sensitive, more callbacks)
 */
#define ANALOG_THRESHOLD 10

/*============================================================================
 * INPUT DEVICE TYPES
 *==========================================================================*/

typedef enum {
    INPUT_TYPE_BUTTON,
    INPUT_TYPE_ANALOG
} InputType;

/*============================================================================
 * BUTTON STATE
 *==========================================================================*/

typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED = 1
} ButtonState;

/*============================================================================
 * CALLBACK TYPES
 *==========================================================================*/

// Forward declaration for callback types
typedef struct InputDevice InputDevice;

/**
 * Callback function type for input events
 * @param device Pointer to the input device that triggered the event
 */
typedef void (*InputCallback)(InputDevice* device);

/*============================================================================
 * INPUT DEVICE STRUCTURE (Polymorphic Interface)
 *==========================================================================*/

/**
 * Base InputDevice structure
 * Provides polymorphic interface for different input types via function pointer
 */
struct InputDevice {
    InputType type;                         // Device type (button or analog)
    void (*poll_impl)(InputDevice* self);   // Polymorphic poll implementation
    void* device_data;                      // Pointer to type-specific data

    // Current accepted value (filtered)
    uint16_t current_value;                 // Button: 0 or 1; Analog: 0-1023

    // Event callbacks (can be NULL)
    InputCallback on_press;                 // Button: triggered on press
    InputCallback on_release;               // Button: triggered on release
    InputCallback on_value_change;          // Analog: triggered when value changes beyond threshold
};

/*============================================================================
 * BUTTON DEVICE DATA
 *==========================================================================*/

/**
 * Button-specific data structure
 */
typedef struct {
    volatile PORT_t* port;                  // Port register (e.g., &PORTC)
    uint8_t pin_bm;                         // Pin bitmask (e.g., PIN4_bm)
    uint8_t active_low;                     // 1 if button is active-low (typical with pull-up)
    ButtonState last_state;                 // Last debounced state
} ButtonData;

/*============================================================================
 * ANALOG DEVICE DATA
 *==========================================================================*/

/**
 * Analog input device data structure
 * Uses relative threshold - callbacks fire when value changes by threshold amount
 */
typedef struct {
    uint8_t adc_channel;                    // ADC channel number (0-14 for ATtiny1627)
    uint16_t last_accepted_value;           // Last value that passed threshold check
    uint16_t threshold;                     // Minimum change to trigger on_value_change
} AnalogData;

/*============================================================================
 * CONSTRUCTORS
 *==========================================================================*/

/**
 * Create a button input device
 * Configures GPIO pin with optional pull-up resistor
 *
 * @param port Port register pointer (e.g., &PORTC)
 * @param pin_bm Pin bitmask (e.g., PIN4_bm for PC4)
 * @param active_low 1 for active-low (typical with pull-up), 0 for active-high
 * @param on_press Callback for button press event (can be NULL)
 * @param on_release Callback for button release event (can be NULL)
 * @return Pointer to new InputDevice, or NULL on allocation failure
 */
InputDevice* create_button(volatile PORT_t* port, uint8_t pin_bm,
                          uint8_t active_low,
                          InputCallback on_press,
                          InputCallback on_release);

/**
 * Create an analog input device (ADC)
 * Uses relative threshold - callback fires when value changes by threshold amount
 *
 * @param adc_channel ADC channel number (0-14)
 * @param threshold Minimum change to trigger callback (e.g., 10 = 1% of 1023 range)
 * @param on_value_change Callback when value changes beyond threshold (can be NULL)
 * @return Pointer to new InputDevice, or NULL on allocation failure
 */
InputDevice* create_analog(uint8_t adc_channel, uint16_t threshold,
                          InputCallback on_value_change);

/*============================================================================
 * DEVICE OPERATIONS
 *==========================================================================*/

/**
 * Poll an input device (read hardware and trigger callbacks if needed)
 * Call once per frame in game loop for each input device
 *
 * For buttons: Reads GPIO pin state and detects press/release events
 * For analog: Performs ADC conversion and checks threshold
 *
 * @param device Pointer to input device
 */
void poll_input(InputDevice* device);

/**
 * Get current value of input device
 * Returns the last accepted (filtered) value
 *
 * @param device Pointer to input device
 * @return Current value (button: 0 or 1, analog: 0-1023)
 */
uint16_t get_input_value(InputDevice* device);

/**
 * Destroy an input device and free memory
 * @param device Pointer to input device to destroy
 */
void destroy_input_device(InputDevice* device);

/*============================================================================
 * HARDWARE INITIALIZATION
 *==========================================================================*/

/**
 * Initialize ADC peripheral for analog inputs
 * Configures ADC0 for 10-bit single-ended mode with VDD reference
 * Must be called before creating analog input devices
 *
 * Configuration:
 * - 10-bit resolution (0-1023 range)
 * - VDD voltage reference
 * - CLK_PER / 4 prescaler
 */
void init_adc(void);

#endif // IO_HARDWARE_H
