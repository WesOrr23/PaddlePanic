/*============================================================================
 * game_controller.c - Created by Wes Orr (11/30/25)
 *============================================================================
 * High-level game controller implementation
 *==========================================================================*/

#include "game_controller.h"
#include "shapes.h"
#include <stddef.h>

/*============================================================================
 * COLLISION CALLBACKS
 *==========================================================================*/

void wall_hit(PhysicsObject* self, PhysicsObject* other) {
    collision_none(self, other);  // Walls don't respond
}

void paddle_hit(PhysicsObject* self, PhysicsObject* other) {
    collision_none(self, other);  // Paddles don't respond (for now)
}

void ball_hit(PhysicsObject* self, PhysicsObject* other) {
    collision_bounce(self, other);  // Ball bounces off everything
}

/*============================================================================
 * HELPER FUNCTIONS
 *==========================================================================*/

/**
 * 8 predefined direction vectors for ball movement
 * Using speed of 2 pixels/frame for good gameplay pace
 * Mix of diagonal and angled directions to avoid pure horizontal/vertical
 */
static const Vector2D DIRECTIONS[8] = {
    { 2,  1},  // ENE (≈26°)
    { 1,  2},  // NNE (≈63°)
    {-1,  2},  // NNW (≈117°)
    {-2,  1},  // WNW (≈154°)
    {-2, -1},  // WSW (≈206°)
    {-1, -2},  // SSW (≈243°)
    { 1, -2},  // SSE (≈297°)
    { 2, -1},  // ESE (≈334°)
};

/**
 * Generate random direction for ball
 * Uses ADC noise from joystick as randomness source
 * @param seed Random seed (typically ADC value at button press)
 * @return Random velocity vector from predefined set
 */
static Vector2D generate_random_direction(uint16_t seed) {
    uint8_t index = seed & 0x07;  // Mask to get 0-7 range
    return DIRECTIONS[index];
}

/**
 * Normalize raw 12-bit ADC value to int16_t (-2048 to +2047)
 * Applies deadzone
 */
static int16_t normalize_adc(uint16_t raw_value) {
    const uint16_t CENTER = 2048;

    int16_t delta = (int16_t)raw_value - (int16_t)CENTER;

    // Apply deadzone
    if (delta < 0 && delta > -JOYSTICK_DEADZONE) {
        return 0;
    } else if (delta >= 0 && delta < JOYSTICK_DEADZONE) {
        return 0;
    }

    return delta;
}

/**
 * Map normalized joystick value to screen position
 * @param normalized_value Normalized joystick (-2048 to +2047, 0 at center)
 * @param min_pos Minimum allowed position
 * @param max_pos Maximum allowed position
 * @return Mapped position (uint8_t)
 */
static uint8_t map_to_position(int16_t normalized_value, uint8_t min_pos, uint8_t max_pos) {
    uint8_t range = max_pos - min_pos;
    uint8_t center = min_pos + range / 2;

    // Scale normalized_value (-2048 to +2047) to position offset
    // normalized_value * range / 4095 gives offset from center
    int32_t offset = ((int32_t)normalized_value * (int32_t)range) / 4095;

    int16_t position = center + offset;

    // Clamp to bounds
    if (position < min_pos) position = min_pos;
    if (position > max_pos) position = max_pos;

    return (uint8_t)position;
}

/*============================================================================
 * GAME CONTROLLER INITIALIZATION
 *==========================================================================*/

void init_game_controller(GameController* controller) {
    if (controller == NULL) return;

    // Initialize input controller
    init_input_controller(&controller->input_ctrl);

    // Calculate paddle fixed positions (distance from walls)
    controller->h_paddle_y_top = WALL_THICKNESS + PADDLE_MARGIN + PADDLE_WIDTH/2;
    controller->h_paddle_y_bottom = SCREEN_HEIGHT - 1 - WALL_THICKNESS - PADDLE_MARGIN - PADDLE_WIDTH/2;
    controller->v_paddle_x_left = WALL_THICKNESS + PADDLE_MARGIN + PADDLE_WIDTH/2;
    controller->v_paddle_x_right = SCREEN_WIDTH - 1 - WALL_THICKNESS - PADDLE_MARGIN - PADDLE_WIDTH/2;

    // Calculate paddle movement bounds (accounting for other paddles to prevent overlap)
    // Horizontal paddles: must not overlap with vertical paddles
    controller->h_paddle_min_x = controller->v_paddle_x_left + PADDLE_WIDTH/2 + PADDLE_LENGTH/2;
    controller->h_paddle_max_x = controller->v_paddle_x_right - PADDLE_WIDTH/2 - PADDLE_LENGTH/2;

    // Vertical paddles: must not overlap with horizontal paddles
    controller->v_paddle_min_y = controller->h_paddle_y_top + PADDLE_WIDTH/2 + PADDLE_LENGTH/2;
    controller->v_paddle_max_y = controller->h_paddle_y_bottom - PADDLE_WIDTH/2 - PADDLE_LENGTH/2;

    // Create walls
    init_physics(&controller->walls[0], (Point){0, 0}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {SCREEN_WIDTH, WALL_THICKNESS, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    init_physics(&controller->walls[1], (Point){0, SCREEN_HEIGHT - WALL_THICKNESS}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {SCREEN_WIDTH, WALL_THICKNESS, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    init_physics(&controller->walls[2], (Point){0, 0}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {WALL_THICKNESS, SCREEN_HEIGHT, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    init_physics(&controller->walls[3], (Point){SCREEN_WIDTH - WALL_THICKNESS, 0}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {WALL_THICKNESS, SCREEN_HEIGHT, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    // Create paddles (all start centered)
    init_physics(&controller->paddles[0], (Point){SCREEN_WIDTH/2, controller->h_paddle_y_top}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_LENGTH, PADDLE_WIDTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    init_physics(&controller->paddles[1], (Point){SCREEN_WIDTH/2, controller->h_paddle_y_bottom}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_LENGTH, PADDLE_WIDTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    init_physics(&controller->paddles[2], (Point){controller->v_paddle_x_left, SCREEN_HEIGHT/2}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_WIDTH, PADDLE_LENGTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    init_physics(&controller->paddles[3], (Point){controller->v_paddle_x_right, SCREEN_HEIGHT/2}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_WIDTH, PADDLE_LENGTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    // Initialize game state
    controller->state = GAME_STATE_BALL_AT_REST;
    controller->button1_prev_state = 0;

    // Create ball (starts at rest in center)
    init_physics(&controller->ball,
                 (Point){SCREEN_WIDTH/2, SCREEN_HEIGHT/2},
                 (Vector2D){0, 0},  // At rest initially
                 SHAPE_CIRCLE,
                 (ShapeParams){.circle = {BALL_RADIUS, 1, COLOR_WHITE}},
                 ball_hit);
}

void destroy_game_controller(GameController* ctrl) {
    if (ctrl == NULL) return;

    // Destroy input controller
    destroy_input_controller(&ctrl->input_ctrl);

    // Destroy game objects
    for (int i = 0; i < 4; i++) {
        destroy(&ctrl->walls[i]);
        destroy(&ctrl->paddles[i]);
    }

    // Destroy ball
    destroy(&ctrl->ball);
}

/*============================================================================
 * GAME CONTROLLER UPDATE
 *==========================================================================*/

void update_game_controller(GameController* ctrl) {
    if (ctrl == NULL) return;

    // Update input controller (polls hardware)
    update_input_controller(&ctrl->input_ctrl);

    // Get raw ADC values
    uint16_t raw_x = input_controller_joystick_x(&ctrl->input_ctrl);  // 0-4095
    uint16_t raw_y = input_controller_joystick_y(&ctrl->input_ctrl);  // 0-4095

    // Normalize (apply deadzone, center at 0) and invert axes for correct control direction
    int16_t norm_x = -normalize_adc(raw_x);  // Inverted: -2048 to +2047
    int16_t norm_y = -normalize_adc(raw_y);  // Inverted: -2048 to +2047

    // Map to pixel positions
    uint8_t h_paddle_x = map_to_position(norm_x, ctrl->h_paddle_min_x, ctrl->h_paddle_max_x);
    uint8_t v_paddle_y = map_to_position(norm_y, ctrl->v_paddle_min_y, ctrl->v_paddle_max_y);

    // Update horizontal paddles (top/bottom - move left/right only)
    set_physics_position(&ctrl->paddles[0], (Point){h_paddle_x, ctrl->h_paddle_y_top});
    set_physics_position(&ctrl->paddles[1], (Point){h_paddle_x, ctrl->h_paddle_y_bottom});

    // Update vertical paddles (left/right - move up/down only)
    set_physics_position(&ctrl->paddles[2], (Point){ctrl->v_paddle_x_left, v_paddle_y});
    set_physics_position(&ctrl->paddles[3], (Point){ctrl->v_paddle_x_right, v_paddle_y});

    // Button edge detection
    uint8_t button1_current = input_controller_button1_pressed(&ctrl->input_ctrl);
    uint8_t button1_pressed = button1_current && !ctrl->button1_prev_state;

    // State machine
    switch (ctrl->state) {
        case GAME_STATE_BALL_AT_REST:
            if (button1_pressed) {
                // Generate random direction using ADC as seed
                uint16_t seed = input_controller_joystick_x(&ctrl->input_ctrl);
                Vector2D velocity = generate_random_direction(seed);
                set_physics_velocity(&ctrl->ball, velocity);
                ctrl->state = GAME_STATE_BALL_MOVING;
            }
            break;

        case GAME_STATE_BALL_MOVING:
            if (button1_pressed) {
                // Reset ball to center, stop movement
                set_physics_position(&ctrl->ball, (Point){SCREEN_WIDTH/2, SCREEN_HEIGHT/2});
                set_physics_velocity(&ctrl->ball, (Vector2D){0, 0});
                ctrl->state = GAME_STATE_BALL_AT_REST;
            } else {
                // Update ball physics (applies velocity to position)
                update(&ctrl->ball);

                // Check collisions with walls
                for (int i = 0; i < 4; i++) {
                    check_collision(&ctrl->ball, &ctrl->walls[i]);
                }

                // Check collisions with paddles
                for (int i = 0; i < 4; i++) {
                    check_collision(&ctrl->ball, &ctrl->paddles[i]);
                }
            }
            break;
    }

    // Update button previous state
    ctrl->button1_prev_state = button1_current;
}

/*============================================================================
 * GAME CONTROLLER DRAWING
 *==========================================================================*/

void draw_game_controller(GameController* ctrl) {
    if (ctrl == NULL) return;

    // Draw walls
    for (int i = 0; i < 4; i++) {
        draw(ctrl->walls[i].visual);
    }

    // Draw paddles
    for (int i = 0; i < 4; i++) {
        draw(ctrl->paddles[i].visual);
    }

    // Draw ball
    draw(ctrl->ball.visual);

    // Future: Draw score, etc.
}
