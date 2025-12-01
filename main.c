/*
 * main.c - PaddlePanic Game
 *
 * Created: 10/27/2024 9:03:11 PM
 * Author: Wes Orr
 */
#include <xc.h>
#include "sh1106_graphics.h"
#include "game_controller.h"
#include "text.h"

int main(void) {
    // Initialize display (includes SPI setup)
    initScreen();

    // Initialize game controller (creates all objects and input controller)
    GameController game;
    init_game_controller(&game);

    // Game loop
    while (1) {
        // Update game state (input, paddle positions, collisions)
        update_game_controller(&game);

        // Render
        clearDisplay();
        draw_game_controller(&game);

        // Draw score in top-left corner
        drawNumber(5, 5, game.score, COLOR_WHITE);

        refreshDisplay();
    }

    // Cleanup (never reached)
    destroy_game_controller(&game);
    return 0;
}
