/*============================================================================
 * game_controller.h - Created by Wes Orr (11/30/25)
 *============================================================================
 * High-level game controller for PaddlePanic
 *
 * Owns all game objects (walls, paddles, ball), input controller, and
 * implements game logic including:
 * - Position-based paddle control with constraints
 * - Collision detection and response
 * - Input interpretation (raw ADC → normalized → pixel position)
 *
 * Architecture:
 *     main.c (orchestration)
 *         ↓
 *     game_controller.c (game logic)
 *         ↓
 *     input_controller.c (hardware input)
 *         ↓
 *     io_hardware.c (low-level devices)
 *==========================================================================*/

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <stdint.h>
#include "physics.h"
#include "input_controller.h"

/*============================================================================
 * GAME CONFIGURATION
 *==========================================================================*/

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

// Game layout constants
#define WALL_THICKNESS 2
#define PADDLE_LENGTH 20
#define PADDLE_WIDTH 2
#define PADDLE_MARGIN 3
#define BALL_RADIUS 3

// Joystick configuration
#define JOYSTICK_DEADZONE 10  // Raw ADC units (out of 4095)

// Paddle velocity configuration
#define MAX_PADDLE_SPEED 8              // Maximum paddle speed (pixels per frame)
#define PADDLE_SPEED_BOOST_MULTIPLIER 2 // Speed multiplier when joystick button pressed

// Collision configuration
#define PADDLE_COLLISION_COOLDOWN_FRAMES 8  // Frames to wait before allowing paddle collision again

// Velocity curve breakpoints (piecewise linear mapping)
#define PADDLE_DEFLECTION_LOW  512      // 25% joystick deflection (0-2048 range)
#define PADDLE_DEFLECTION_MID  1536     // 75% joystick deflection (0-2048 range)

#define PADDLE_SPEED_LOW   2            // Speed at 25% deflection (pixels per frame)
#define PADDLE_SPEED_MID   4            // Speed at 75% deflection (pixels per frame)
#define PADDLE_SPEED_HIGH  8            // Speed at 100% deflection (pixels per frame)

/*============================================================================
 * GAME CONTROLLER STRUCTURE
 *==========================================================================*/

/**
 * Game state enumeration
 * Tracks ball state for state machine
 */
typedef enum {
    GAME_STATE_BALL_AT_REST,   // Ball is stationary at center
    GAME_STATE_BALL_MOVING     // Ball is moving and bouncing
} GameState;

/**
 * GameController structure
 * Owns all game objects and input controller
 */
typedef struct {
    // Input management
    InputController input_ctrl;

    // Game objects
    PhysicsObject walls[4];
    PhysicsObject paddles[4];

    // Paddle constraints (fixed positions and movement bounds)
    // Horizontal paddles (top/bottom)
    uint8_t h_paddle_y_top;      // Fixed Y position for top paddle
    uint8_t h_paddle_y_bottom;   // Fixed Y position for bottom paddle
    uint8_t h_paddle_min_x;      // Min X position (left bound)
    uint8_t h_paddle_max_x;      // Max X position (right bound)

    // Vertical paddles (left/right)
    uint8_t v_paddle_x_left;     // Fixed X position for left paddle
    uint8_t v_paddle_x_right;    // Fixed X position for right paddle
    uint8_t v_paddle_min_y;      // Min Y position (top bound)
    uint8_t v_paddle_max_y;      // Max Y position (bottom bound)

    // Game state
    GameState state;

    // Ball object
    PhysicsObject ball;

    // Score tracking
    uint16_t score;

    // Collision cooldown per paddle (prevents paddle trapping)
    uint8_t paddle_collision_cooldown[4];  // One cooldown per paddle

    // Button edge detection
    uint8_t button1_prev_state;
} GameController;

/*============================================================================
 * GAME CONTROLLER OPERATIONS
 *==========================================================================*/

/**
 * Initialize game controller
 * Creates all game objects (walls, paddles) and input controller
 * @param ctrl Pointer to game controller
 */
void init_game_controller(GameController* ctrl);

/**
 * Destroy game controller
 * Cleans up all game objects and input controller
 * @param ctrl Pointer to game controller
 */
void destroy_game_controller(GameController* ctrl);

/**
 * Update game state
 * - Polls input controller
 * - Updates paddle positions based on joystick
 * - Checks collisions (when ball exists)
 *
 * Call once per frame in game loop
 * @param ctrl Pointer to game controller
 */
void update_game_controller(GameController* ctrl);

/**
 * Draw all game objects
 * Call between clearDisplay() and refreshDisplay()
 * @param ctrl Pointer to game controller
 */
void draw_game_controller(GameController* ctrl);

#endif // GAME_CONTROLLER_H
