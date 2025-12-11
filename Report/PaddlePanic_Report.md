# PaddlePanic: Embedded Pong Game

## System Overview

This system employs an ATtiny1627 microcontroller interfaced with an SH1106 128×64 monochrome OLED display and analog joystick to create a single-player Pong variant where the player controls all four paddles simultaneously. The game utilizes a position-based physics engine with collision detection, smooth paddle acceleration via piecewise-linear joystick mapping, and a six-state controller managing gameplay flow. This demonstrates real-time embedded game development with layered software architecture, custom graphics rendering, and polling-based input handling optimized for limited microcontroller resources.

## Hardware

The hardware comprises an ATtiny1627 microcontroller, an SH1106 OLED display (128×64 monochrome, SPI interface), a dual-axis analog joystick with integrated push-button, and an onboard tactile button. The SH1106 connects via SPI on PC0 (CLK), PC2 (MOSI), PC3 (CS), PB1 (DC), and PB0 (RES). The joystick X-axis connects to PA1 (ADC channel 1), Y-axis to PA2 (ADC channel 2), with buttons on PC5 (joystick button) and PC4 (onboard button), both configured as active-low inputs with internal pull-up resistors. The ADC operates in 12-bit mode providing 0-4095 range. Power is supplied at 3.3V.

![Hardware Schematic](Paddle%20Panic%20Schematic.png)

## Software

The program employs a layered architecture separating hardware abstraction, input processing, physics simulation, graphics rendering, and game logic. Initialization configures SPI in master mode, ADC in 12-bit single-ended mode, and GPIO pins with pull-up resistors. The input controller abstracts hardware devices through polymorphic InputDevice structures, polling buttons and analog axes each frame to provide raw ADC values and debounced button states.

The game controller implements a six-state machine (TITLE, BALL_AT_REST, BALL_MOVING, PAUSED, COUNTDOWN, GAME_OVER) managing game flow. Raw joystick inputs undergo normalization (centering at 2048, applying ±10 ADC unit deadzone) then piecewise-linear mapping: 0-25% deflection maps to 0-2 px/frame, 25-75% to 2-4 px/frame, and 75-100% to 4-8 px/frame maximum target velocity. A smooth acceleration function applies ±2 px/frame² changes to prevent instantaneous movement. Paddle positions are clamped to prevent overlap, with per-paddle 8-frame collision cooldown preventing ball trapping.

The physics engine uses position-based updates with integer velocity vectors. Collision detection employs axis-aligned bounding box tests between shapes, invoking callbacks on contact. Ball direction initializes from one of eight predefined velocity vectors, selected via ADC noise sampling for pseudo-randomness. Score increments on paddle hits when cooldown is zero, and wall collision triggers display inversion flash followed by game over.

The graphics system maintains a 1KB framebuffer and provides pixel, line (Bresenham's algorithm), and bitmap primitives. Text rendering uses custom 3×5 pixel bitmap fonts with integer scaling. The display refreshes via page-addressed SPI transfers, achieving approximately 12 fps limited by SPI communication overhead. The main loop polls input, updates physics, clears the framebuffer, renders state-specific screens, and transmits to hardware.

The full code is available at https://github.com/WesOrr23/PaddlePanic.git. See below for graphical flow diagram. A textual version that is more detailed is included

![Flow Diagram](Paddle%20Panic%20Flow%20Diagram.png)

## System Behavior

Upon power-up, the microcontroller initializes peripherals and enters TITLE state, displaying "PADDLE PANIC" and "PRESS START" on the OLED. Pressing the primary button resets score, centers paddles and ball, and transitions to BALL_AT_REST. During BALL_AT_REST, the player positions paddles via joystick (left/right controls horizontal paddles, up/down controls vertical) while the ball remains stationary. Pressing the button samples joystick ADC as entropy, selects a velocity vector, and transitions to BALL_MOVING.

In BALL_MOVING, the physics engine updates ball position each frame, checking collisions with paddles and walls. Paddle collisions increment score, reverse ball velocity, and reset that paddle's 8-frame cooldown. Wall collision inverts the display briefly, saves final score, and transitions to GAME_OVER. Pressing the button during gameplay saves ball velocity, stops movement, and transitions to PAUSED, overlaying a bordered rectangle with score display.

From PAUSED, pressing the button initializes countdown_timer to 36 frames and transitions to COUNTDOWN, displaying large numbers (3, 2, 1) centered on screen. When the timer reaches zero, ball velocity restores and gameplay resumes. In GAME_OVER, "GAME OVER" displays with final score; pressing the button returns to TITLE.

Throughout gameplay, paddle velocity updates continuously via joystick input with normalization, piecewise-linear mapping, optional 2× speed boost when joystick button is held, and smooth acceleration. Horizontal paddles share X-velocity, vertical paddles share Y-velocity, with positions clamped to bounds. All collision cooldowns decrement each frame when non-zero.

## Potential Enhancements

**Migrate to Color Display with Higher Refresh Rate**: During development, an attempt was made to upgrade from the SH1106 monochrome OLED to an ST7789 240×280 16-bit color TFT display. This migration introduced substantial challenges due to the ATtiny1627's limited SPI throughput and SRAM constraints. The ST7789 requires transmitting 2 bytes per pixel (RGB565 format) totaling 134,400 bytes, far exceeding the ATtiny1627's 2KB SRAM. Even with aggressive optimizations including partial framebuffer updates, direct SPI writes without buffering, and object-differential rendering, the SPI bottleneck persisted. At the maximum achievable SPI clock of 1.67 MHz (limited by the ATtiny1627's peripheral constraints), full-screen refresh times remained prohibitively long, degrading gameplay responsiveness. The lesson learned is that certain hardware upgrades demand corresponding increases in processing capability and memory bandwidth; while software optimization can stretch performance, fundamental hardware limitations eventually dictate feasibility. For this use case, migrating to a more capable microcontroller (e.g., ESP32 with DMA-enabled SPI and sufficient SRAM) or accepting the monochrome display's constraints represents the pragmatic solution.

**Add Piezo Speaker for Auditory Feedback**: Integrate a piezo buzzer connected to a PWM-capable GPIO pin to provide distinct sound effects for paddle hits, wall collisions, and countdown beeps, enhancing user experience with minimal hardware cost.

**Implement Progressive Difficulty Scaling**: Introduce dynamic ball velocity scaling based on score milestones (e.g., increase velocity every 10 paddle hits, capped at 5 px/frame) to extend gameplay engagement by gradually increasing challenge.

## Conclusion

This project demonstrates embedded real-time game development on the ATtiny1627 microcontroller, showcasing layered software architecture, custom graphics pipeline, and optimized input processing. The primary challenge involved balancing responsiveness with limited hardware resources, addressed through smooth acceleration mechanics, efficient framebuffer operations, and polling-based input management. The piecewise-linear joystick mapping and per-paddle collision cooldown exemplify domain-specific solutions to gameplay feel and physics edge cases. The attempted migration to a color display highlighted a critical embedded systems principle: hardware capabilities define performance boundaries, and optimization cannot compensate indefinitely for insufficient processing power or memory bandwidth. The resulting game successfully delivers smooth paddle control, predictable physics, and complete state-driven user interaction, validating the architecture's effectiveness within the ATtiny1627's constraints.
