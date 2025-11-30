/*============================================================================
 * io_hardware.c - Created by Wes Orr (11/30/25)
 *============================================================================
 * Low-level hardware input abstraction implementation
 *==========================================================================*/

#include "io_hardware.h"
#include <stdlib.h>

/*============================================================================
 * BUTTON IMPLEMENTATION
 *==========================================================================*/

/**
 * Internal button polling function
 * Reads GPIO pin state and detects press/release events
 */
static void poll_button(InputDevice* self) {
    if (self == NULL || self->device_data == NULL) return;

    ButtonData* data = (ButtonData*)self->device_data;

    // Read physical pin state
    uint8_t pin_level = (data->port->IN & data->pin_bm) ? 1 : 0;

    // Apply active-low logic if needed
    uint8_t logical_state = data->active_low ? !pin_level : pin_level;

    ButtonState current_state = logical_state ? BUTTON_PRESSED : BUTTON_RELEASED;

    // Detect state change
    if (current_state != data->last_state) {
        data->last_state = current_state;
        self->current_value = current_state;

        // Trigger appropriate callback
        if (current_state == BUTTON_PRESSED && self->on_press != NULL) {
            self->on_press(self);
        } else if (current_state == BUTTON_RELEASED && self->on_release != NULL) {
            self->on_release(self);
        }
    }
}

/**
 * Create a button input device
 */
InputDevice* create_button(volatile PORT_t* port, uint8_t pin_bm,
                          uint8_t active_low,
                          InputCallback on_press,
                          InputCallback on_release) {
    // Allocate InputDevice
    InputDevice* device = (InputDevice*)malloc(sizeof(InputDevice));
    if (device == NULL) return NULL;

    // Allocate ButtonData
    ButtonData* data = (ButtonData*)malloc(sizeof(ButtonData));
    if (data == NULL) {
        free(device);
        return NULL;
    }

    // Configure GPIO hardware
    port->DIRCLR = pin_bm;  // Set pin as input

    if (active_low) {
        // Enable internal pull-up for active-low buttons
        // Calculate pin index from bitmask
        uint8_t pin_index = 0;
        uint8_t mask = pin_bm;
        while (mask > 1) {
            mask >>= 1;
            pin_index++;
        }

        // Access PINnCTRL register for this pin
        volatile uint8_t* pin_ctrl = (volatile uint8_t*)&port->PIN0CTRL;
        pin_ctrl[pin_index] = PORT_PULLUPEN_bm;
    }

    // Initialize ButtonData
    data->port = port;
    data->pin_bm = pin_bm;
    data->active_low = active_low;
    data->last_state = BUTTON_RELEASED;

    // Initialize InputDevice
    device->type = INPUT_TYPE_BUTTON;
    device->poll_impl = poll_button;
    device->device_data = data;
    device->current_value = 0;
    device->on_press = on_press;
    device->on_release = on_release;
    device->on_value_change = NULL;  // Not used for buttons

    return device;
}

/*============================================================================
 * ANALOG IMPLEMENTATION
 *==========================================================================*/

/**
 * Internal analog polling function
 * Performs ADC conversion and checks relative threshold
 */
static void poll_analog(InputDevice* self) {
    if (self == NULL || self->device_data == NULL) return;

    AnalogData* data = (AnalogData*)self->device_data;

    // Select ADC channel
    ADC0.MUXPOS = data->adc_channel;

    // Start conversion
    ADC0.COMMAND = ADC_START_IMMEDIATE_gc;

    // Wait for conversion to complete
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));

    // Read result (10-bit: 0-1023)
    uint16_t raw_value = ADC0.RESULT;

    // Check relative threshold (has value changed significantly?)
    int16_t delta = (int16_t)raw_value - (int16_t)data->last_accepted_value;

    if (delta < 0) delta = -delta;  // abs(delta)

    if (delta >= data->threshold) {
        // Value changed significantly - accept it
        data->last_accepted_value = raw_value;
        self->current_value = raw_value;

        // Trigger callback
        if (self->on_value_change != NULL) {
            self->on_value_change(self);
        }
    }
}

/**
 * Create an analog input device
 */
InputDevice* create_analog(uint8_t adc_channel, uint16_t threshold,
                          InputCallback on_value_change) {
    // Allocate InputDevice
    InputDevice* device = (InputDevice*)malloc(sizeof(InputDevice));
    if (device == NULL) return NULL;

    // Allocate AnalogData
    AnalogData* data = (AnalogData*)malloc(sizeof(AnalogData));
    if (data == NULL) {
        free(device);
        return NULL;
    }

    // Initialize AnalogData
    data->adc_channel = adc_channel;
    data->last_accepted_value = 512;  // Start at center (10-bit midpoint)
    data->threshold = threshold;

    // Initialize InputDevice
    device->type = INPUT_TYPE_ANALOG;
    device->poll_impl = poll_analog;
    device->device_data = data;
    device->current_value = 512;  // Start at center
    device->on_press = NULL;      // Not used for analog
    device->on_release = NULL;
    device->on_value_change = on_value_change;

    return device;
}

/*============================================================================
 * COMMON OPERATIONS
 *==========================================================================*/

void poll_input(InputDevice* device) {
    if (device != NULL && device->poll_impl != NULL) {
        device->poll_impl(device);
    }
}

uint16_t get_input_value(InputDevice* device) {
    return (device != NULL) ? device->current_value : 0;
}

void destroy_input_device(InputDevice* device) {
    if (device != NULL) {
        if (device->device_data != NULL) {
            free(device->device_data);
        }
        free(device);
    }
}

/*============================================================================
 * ADC INITIALIZATION
 *==========================================================================*/

/**
 * Initialize ADC peripheral for analog inputs
 * Configures ADC0 for 10-bit single-ended mode
 */
void init_adc(void) {
    // Reference: ATtiny1627 Datasheet Section 30 (ADC)
    // Based on working ADC example code

    // Enable ADC (10-bit is default resolution)
    ADC0.CTRLA = ADC_ENABLE_bm;

    // Configure ADC prescaler (CLK_PER / 4)
    ADC0.CTRLB = ADC_PRESC_DIV4_gc;

    // Configure voltage reference
    // Using internal 4.096V reference (same as working example)
    // TODO: May need to adjust to VDD reference for joystick
    ADC0.CTRLC = ADC_REFSEL_4096MV_gc;
}
