/*
 * main.c - PaddlePanic Game (ST7789 240x240 version)
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 * Ported to ST7789: 12/9/2025
 */
#include <xc.h>
#include "st7789_graphics.h"
#include "game_controller.h"

// SPI initialization is now in st7789_graphics.h

int main(void) {
    // Initialize SPI peripheral
    initSPI();

    // Initialize ST7789 display
    st7789_init();

    // Clear screen once at startup
    st7789_fillScreen(st7789_grayscale(0));

    // Initialize game controller (creates all objects and input controller)
    GameController game;
    init_game_controller(&game);

    // Game loop
    while (1) {
        // Update game state (input, paddle positions, collisions)
        update_game_controller(&game);

        // Render
        // Note: ST7789 draws directly, no buffer clear/refresh needed
        // We'll need differential rendering for smooth gameplay, but for now
        // the game will render with visible updates (the delay effect you love!)
        draw_game_controller(&game);

        // Frame rate limiter - slow down game loop to keep physics and rendering in sync
        // Without this, physics runs much faster than display can update, causing
        // collision detection to trigger before user sees the ball reach the wall
        // Adjust this delay to tune game speed (smaller = faster, larger = slower)
        for (volatile uint32_t i = 0; i < 500; i++) {}
    }

    // Cleanup (never reached)
    destroy_game_controller(&game);
    return 0;
}
