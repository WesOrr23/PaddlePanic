/*============================================================================
 * game_controller.c - Created by Wes Orr (11/30/25)
 *============================================================================
 * High-level game controller implementation
 *==========================================================================*/

#include "game_controller.h"
#include "shapes.h"
#include "sh1106_graphics.h"
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
 * Map normalized joystick value to paddle velocity with custom curve
 * Piecewise linear mapping using configurable breakpoints
 * @param normalized_value Normalized joystick (-2048 to +2047, 0 at center)
 * @return Velocity (±MAX_PADDLE_SPEED pixels per frame)
 */
static int8_t map_to_velocity(int16_t normalized_value) {
    // Preserve sign
    int8_t sign = (normalized_value < 0) ? -1 : 1;
    int16_t abs_value = (normalized_value < 0) ? -normalized_value : normalized_value;

    int32_t velocity;

    // Piecewise linear mapping with configurable breakpoints
    if (abs_value < PADDLE_DEFLECTION_LOW) {
        // 0% to 25%: maps to 0 to PADDLE_SPEED_LOW
        velocity = (abs_value * PADDLE_SPEED_LOW) / PADDLE_DEFLECTION_LOW;
    } else if (abs_value < PADDLE_DEFLECTION_MID) {
        // 25% to 75%: maps to PADDLE_SPEED_LOW to PADDLE_SPEED_MID
        velocity = PADDLE_SPEED_LOW +
                   ((abs_value - PADDLE_DEFLECTION_LOW) * (PADDLE_SPEED_MID - PADDLE_SPEED_LOW)) /
                   (PADDLE_DEFLECTION_MID - PADDLE_DEFLECTION_LOW);
    } else {
        // 75% to 100%: maps to PADDLE_SPEED_MID to PADDLE_SPEED_HIGH
        velocity = PADDLE_SPEED_MID +
                   ((abs_value - PADDLE_DEFLECTION_MID) * (PADDLE_SPEED_HIGH - PADDLE_SPEED_MID)) /
                   (2048 - PADDLE_DEFLECTION_MID);
    }

    // Clamp to max speed (safety)
    if (velocity > MAX_PADDLE_SPEED) velocity = MAX_PADDLE_SPEED;

    // Restore sign
    return (int8_t)(velocity * sign);
}

/**
 * Clamp paddle position to bounds
 * @param paddle Pointer to paddle physics object
 * @param min_coord Minimum allowed coordinate
 * @param max_coord Maximum allowed coordinate
 * @param is_horizontal True for X clamping (horizontal paddles), False for Y clamping (vertical paddles)
 */
static void clamp_paddle(PhysicsObject* paddle, uint8_t min_coord, uint8_t max_coord, uint8_t is_horizontal) {
    Point pos = get_physics_position(paddle);

    if (is_horizontal) {
        // Clamp X coordinate
        if (pos.x < min_coord) pos.x = min_coord;
        if (pos.x > max_coord) pos.x = max_coord;
    } else {
        // Clamp Y coordinate
        if (pos.y < min_coord) pos.y = min_coord;
        if (pos.y > max_coord) pos.y = max_coord;
    }

    set_physics_position(paddle, pos);
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
    controller->score = 0;
    for (int i = 0; i < 4; i++) {
        controller->paddle_collision_cooldown[i] = 0;
    }
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

    // Map to velocities
    int8_t velocity_x = map_to_velocity(norm_x);  // ±MAX_PADDLE_SPEED pixels/frame
    int8_t velocity_y = map_to_velocity(norm_y);  // ±MAX_PADDLE_SPEED pixels/frame

    // Apply speed boost if joystick button pressed
    uint8_t button2_pressed = input_controller_button2_pressed(&ctrl->input_ctrl);
    if (button2_pressed) {
        velocity_x *= PADDLE_SPEED_BOOST_MULTIPLIER;
        velocity_y *= PADDLE_SPEED_BOOST_MULTIPLIER;

        // Clamp to prevent overflow
        if (velocity_x > 127) velocity_x = 127;
        if (velocity_x < -127) velocity_x = -127;
        if (velocity_y > 127) velocity_y = 127;
        if (velocity_y < -127) velocity_y = -127;
    }

    // Set paddle velocities (horizontal paddles move left/right, vertical paddles move up/down)
    set_physics_velocity(&ctrl->paddles[0], (Vector2D){velocity_x, 0});  // Top paddle
    set_physics_velocity(&ctrl->paddles[1], (Vector2D){velocity_x, 0});  // Bottom paddle
    set_physics_velocity(&ctrl->paddles[2], (Vector2D){0, velocity_y});  // Left paddle
    set_physics_velocity(&ctrl->paddles[3], (Vector2D){0, velocity_y});  // Right paddle

    // Update paddle positions (apply velocity)
    update(&ctrl->paddles[0]);
    update(&ctrl->paddles[1]);
    update(&ctrl->paddles[2]);
    update(&ctrl->paddles[3]);

    // Clamp paddles to bounds
    clamp_paddle(&ctrl->paddles[0], ctrl->h_paddle_min_x, ctrl->h_paddle_max_x, 1);  // Horizontal
    clamp_paddle(&ctrl->paddles[1], ctrl->h_paddle_min_x, ctrl->h_paddle_max_x, 1);  // Horizontal
    clamp_paddle(&ctrl->paddles[2], ctrl->v_paddle_min_y, ctrl->v_paddle_max_y, 0);  // Vertical
    clamp_paddle(&ctrl->paddles[3], ctrl->v_paddle_min_y, ctrl->v_paddle_max_y, 0);  // Vertical

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
                for (int i = 0; i < 4; i++) {
                    ctrl->paddle_collision_cooldown[i] = 0;  // Reset all cooldowns
                }
                ctrl->state = GAME_STATE_BALL_AT_REST;
            } else {
                // Update ball physics (applies velocity to position)
                update(&ctrl->ball);

                // Check collisions with paddles first - increment score
                // Check per-paddle cooldown (prevents paddle trapping)
                for (int i = 0; i < 4; i++) {
                    if (ctrl->paddle_collision_cooldown[i] == 0) {
                        if (check_collision(&ctrl->ball, &ctrl->paddles[i])) {
                            ctrl->score++;
                            ctrl->paddle_collision_cooldown[i] = PADDLE_COLLISION_COOLDOWN_FRAMES;
                            // Don't break - ball can hit multiple paddles in corners
                        }
                    }
                }

                // Check collisions with walls - game over
                for (int i = 0; i < 4; i++) {
                    if (check_collision(&ctrl->ball, &ctrl->walls[i])) {
                        // Game over - flash screen
                        invertDisplay(1);
                        for (volatile uint32_t delay = 0; delay < 50000; delay++) {}
                        invertDisplay(0);

                        // Reset ball to center
                        set_physics_position(&ctrl->ball, (Point){SCREEN_WIDTH/2, SCREEN_HEIGHT/2});
                        set_physics_velocity(&ctrl->ball, (Vector2D){0, 0});
                        for (int j = 0; j < 4; j++) {
                            ctrl->paddle_collision_cooldown[j] = 0;  // Reset all cooldowns
                        }
                        ctrl->state = GAME_STATE_BALL_AT_REST;
                        ctrl->score = 0;
                        break;
                    }
                }
            }
            break;
    }

    // Update button previous state
    ctrl->button1_prev_state = button1_current;

    // Decrement collision cooldowns for all paddles
    for (int i = 0; i < 4; i++) {
        if (ctrl->paddle_collision_cooldown[i] > 0) {
            ctrl->paddle_collision_cooldown[i]--;
        }
    }
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
