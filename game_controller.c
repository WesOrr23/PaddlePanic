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

/*============================================================================
 * HELPER FUNCTIONS
 *==========================================================================*/

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

void init_game_controller(GameController* ctrl) {
    if (ctrl == NULL) return;

    // Initialize input controller
    init_input_controller(&ctrl->input_ctrl);

    // Calculate paddle fixed positions (distance from walls)
    ctrl->h_paddle_y_top = WALL_THICKNESS + PADDLE_MARGIN + PADDLE_WIDTH/2;
    ctrl->h_paddle_y_bottom = SCREEN_HEIGHT - 1 - WALL_THICKNESS - PADDLE_MARGIN - PADDLE_WIDTH/2;
    ctrl->v_paddle_x_left = WALL_THICKNESS + PADDLE_MARGIN + PADDLE_WIDTH/2;
    ctrl->v_paddle_x_right = SCREEN_WIDTH - 1 - WALL_THICKNESS - PADDLE_MARGIN - PADDLE_WIDTH/2;

    // Calculate paddle movement bounds (accounting for other paddles to prevent overlap)
    // Horizontal paddles: must not overlap with vertical paddles
    ctrl->h_paddle_min_x = ctrl->v_paddle_x_left + PADDLE_WIDTH/2 + PADDLE_LENGTH/2;
    ctrl->h_paddle_max_x = ctrl->v_paddle_x_right - PADDLE_WIDTH/2 - PADDLE_LENGTH/2;

    // Vertical paddles: must not overlap with horizontal paddles
    ctrl->v_paddle_min_y = ctrl->h_paddle_y_top + PADDLE_WIDTH/2 + PADDLE_LENGTH/2;
    ctrl->v_paddle_max_y = ctrl->h_paddle_y_bottom - PADDLE_WIDTH/2 - PADDLE_LENGTH/2;

    // Create walls
    init_physics(&ctrl->walls[0], (Point){0, 0}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {SCREEN_WIDTH, WALL_THICKNESS, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    init_physics(&ctrl->walls[1], (Point){0, SCREEN_HEIGHT - WALL_THICKNESS}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {SCREEN_WIDTH, WALL_THICKNESS, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    init_physics(&ctrl->walls[2], (Point){0, 0}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {WALL_THICKNESS, SCREEN_HEIGHT, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    init_physics(&ctrl->walls[3], (Point){SCREEN_WIDTH - WALL_THICKNESS, 0}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {WALL_THICKNESS, SCREEN_HEIGHT, ANCHOR_TOP_LEFT, 1, COLOR_WHITE}},
                 wall_hit);

    // Create paddles (all start centered)
    init_physics(&ctrl->paddles[0], (Point){SCREEN_WIDTH/2, ctrl->h_paddle_y_top}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_LENGTH, PADDLE_WIDTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    init_physics(&ctrl->paddles[1], (Point){SCREEN_WIDTH/2, ctrl->h_paddle_y_bottom}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_LENGTH, PADDLE_WIDTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    init_physics(&ctrl->paddles[2], (Point){ctrl->v_paddle_x_left, SCREEN_HEIGHT/2}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_WIDTH, PADDLE_LENGTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);

    init_physics(&ctrl->paddles[3], (Point){ctrl->v_paddle_x_right, SCREEN_HEIGHT/2}, (Vector2D){0, 0},
                 SHAPE_RECTANGLE,
                 (ShapeParams){.rect = {PADDLE_WIDTH, PADDLE_LENGTH, ANCHOR_CENTER, 1, COLOR_WHITE}},
                 paddle_hit);
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

    // Future: Check collisions with ball
    // for (int i = 0; i < 4; i++) {
    //     check_collision(&ball, &ctrl->walls[i]);
    //     check_collision(&ball, &ctrl->paddles[i]);
    // }
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

    // Future: Draw ball, score, etc.
}
