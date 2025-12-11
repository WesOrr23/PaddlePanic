# PaddlePanic

A fast-paced single-player Pong-style game for embedded systems. Control all four paddles simultaneously to keep the ball in play and rack up points!

![Game Demo](docs/gameplay.gif) <!-- Add your demo GIF here -->

## 📋 Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Features](#features)
- [Controls](#controls)
- [Game Mechanics](#game-mechanics)
- [Building the Project](#building-the-project)
- [Project Structure](#project-structure)
- [Architecture](#architecture)
- [Development](#development)

## 🎮 Overview

PaddlePanic is an embedded game built for the ATtiny1627 microcontroller with an SH1106 OLED display. Unlike traditional Pong, you control all four paddles at once using a single joystick, creating a unique and challenging gameplay experience.

## 🔧 Hardware Requirements

- **Microcontroller**: ATtiny1627 (3.3V)
- **Display**: SH1106 OLED (128×64 monochrome, SPI)
- **Input**:
  - Analog joystick (X/Y axes, 3.3V compatible)
  - 2 push buttons (on-board button + joystick button)
- **Communication**: SPI for display
- **Power**: 3.3V regulated supply

### Pin Configuration

| Function          | Pin   | Notes                           |
|-------------------|-------|---------------------------------|
| Display CLK       | PC0   | SPI clock                       |
| Display MOSI      | PC2   | SPI data to display             |
| Display CS        | PC3   | Chip select (active-low)        |
| Display DC        | PB1   | Data/Command select             |
| Display RES       | PB0   | Reset line                      |
| Joystick X        | PA1   | ADC channel 1 (12-bit)          |
| Joystick Y        | PA2   | ADC channel 2 (12-bit)          |
| Joystick Button   | PC5   | Speed boost (active-low)        |
| Primary Button    | PC4   | Start/Pause/Unpause (active-low)|

## ✨ Features

### Gameplay
- ⚡ Real-time physics engine with collision detection
- 🎯 Four-paddle simultaneous control system
- 📊 Score tracking (paddle hits)
- ⏸️ Pause menu with countdown resume
- 🎲 Random ball launch directions
- 💨 Speed boost mode (hold joystick button)

### Technical
- 🎨 Custom 3×5 pixel bitmap text rendering
- 🎮 Smooth paddle acceleration (2 px/frame²)
- 🔄 Piecewise-linear joystick response curve
- 🛡️ Per-paddle collision cooldown (prevents trapping)
- 📐 Centered joystick deadzone (10 ADC units)
- 🖥️ Complete state machine game loop

## 🕹️ Controls

### Joystick
- **Left/Right**: Move horizontal paddles (top & bottom)
- **Up/Down**: Move vertical paddles (left & right)
- **Hold Joystick Button**: 2× speed boost

### Primary Button
- **Title Screen**: Start game
- **Ball at Rest**: Launch ball
- **During Play**: Pause game
- **Paused**: Resume (with 3-2-1 countdown)
- **Game Over**: Return to title

## 🎯 Game Mechanics

### Objective
Keep the ball in play as long as possible by preventing it from hitting the walls. Each paddle hit increases your score.

### Paddle Movement
- All four paddles move simultaneously based on joystick input
- Smooth acceleration prevents jerky movement
- Speed boost available (hold joystick button)
- Paddles cannot overlap each other

### Ball Physics
- Ball launches in one of 8 predefined directions
- Bounces off paddles (changes velocity)
- Game over when ball hits any wall
- Random direction selection uses ADC noise for entropy

### Collision System
- **Paddle hits**: +1 score, ball bounces
- **Wall hits**: Game over, screen flash, show final score
- **Cooldown**: 8-frame cooldown per paddle prevents ball trapping
- **Corner bounces**: Can hit multiple paddles simultaneously

### Scoring
- +1 point per paddle collision
- Score displayed in pause menu
- Final score shown on game over screen

## 🔨 Building the Project

### Prerequisites
- MPLAB X IDE
- XC8 Compiler (for AVR)
- MPLAB Code Configurator (MCC) - optional

### Build Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/PaddlePanic.git
   cd PaddlePanic
   ```

2. **Open in MPLAB X**
   - Open MPLAB X IDE
   - File → Open Project
   - Select the `PaddlePanic` folder

3. **Build**
   - Click "Build Main Project" (hammer icon)
   - Or press F11

4. **Program Device**
   - Connect your ATtiny1627 programmer
   - Click "Make and Program Device" (play icon)
   - Or press F5

### Build Artifacts
All build outputs are stored in `_build/` directory.

## 📁 Project Structure

### Core Files

| File                    | Purpose                                           |
|-------------------------|---------------------------------------------------|
| `main.c`                | Main game loop orchestration                      |
| `game_controller.c/h`   | High-level game logic and state machine           |
| `input_controller.c/h`  | Input abstraction (joystick + buttons)            |
| `physics.c/h`           | Physics engine (collision, movement)              |
| `shapes.c/h`            | Shape rendering (circles, rectangles)             |
| `sh1106_graphics.c/h`   | Display driver and graphics primitives            |
| `text.c/h`              | Text/number rendering (bitmap fonts)              |
| `io_hardware.c/h`       | Low-level hardware abstraction (ADC, GPIO, SPI)   |

### Architecture Layers

```
┌─────────────────────────────────┐
│         main.c                  │  ← Game loop orchestration
├─────────────────────────────────┤
│    game_controller.c            │  ← Game logic & state machine
├─────────────────────────────────┤
│   input_controller.c            │  ← Input interpretation
│      physics.c                  │  ← Physics & collisions
│      shapes.c                   │  ← Shape abstraction
│      text.c                     │  ← Text rendering
├─────────────────────────────────┤
│   sh1106_graphics.c             │  ← Display driver
│   io_hardware.c                 │  ← Hardware abstraction
└─────────────────────────────────┘
```

### Directory Structure

```
PaddlePanic/
├── README.md                      # This file
├── main.c                         # Entry point
├── game_controller.c/h            # Game logic
├── input_controller.c/h           # Input handling
├── physics.c/h                    # Physics engine
├── shapes.c/h                     # Graphics shapes
├── sh1106_graphics.c/h            # Display driver
├── text.c/h                       # Text rendering
├── io_hardware.c/h                # Hardware layer
├── _build/                        # Build artifacts (generated)
├── cmake/                         # CMake files (generated)
└── .vscode/                       # VSCode settings
    └── PaddlePanic.mplab.json     # MPLAB project file
```

## 🏗️ Architecture

### State Machine

The game uses a six-state machine:

```
TITLE → BALL_AT_REST → BALL_MOVING ⇄ PAUSED → COUNTDOWN
  ↑                           ↓
  └───────── GAME_OVER ←──────┘
```

| State             | Description                                    | Button Action        |
|-------------------|------------------------------------------------|----------------------|
| `TITLE`           | Shows game title and start prompt             | Start game           |
| `BALL_AT_REST`    | Ball stationary at center, paddles active     | Launch ball          |
| `BALL_MOVING`     | Active gameplay with physics and collisions   | Pause game           |
| `PAUSED`          | Game frozen, score displayed                  | Begin countdown      |
| `COUNTDOWN`       | 3-2-1 countdown before resuming               | N/A (auto-resumes)   |
| `GAME_OVER`       | Shows final score, waits for restart          | Return to title      |

### Physics System

- **Position-based**: Shapes have origin points
- **Velocity-based**: Objects move via velocity vectors
- **Frame-based**: Updates occur once per frame (~12 fps)
- **AABB Collision**: Axis-aligned bounding box detection
- **Callbacks**: Custom collision response per object

### Input Pipeline

```
Raw ADC (0-4095)
    ↓
Normalize & Deadzone (-2048 to +2047, deadzone ±10)
    ↓
Piecewise Linear Curve (0-25%-75%-100%)
    ↓
Target Velocity (±8 px/frame max)
    ↓
Smooth Acceleration (±2 px/frame²)
    ↓
Paddle Movement
```

### Rendering Pipeline

```
clearDisplay()
    ↓
draw_game_controller()
    ├── State-specific screens (title/game over)
    ├── Game objects (walls, paddles, ball)
    ├── Pause menu overlay
    └── Countdown numbers
    ↓
refreshDisplay()
```

## 🛠️ Development

### Tunable Parameters

All game constants are defined in `game_controller.h`:

```c
// Layout
#define PADDLE_LENGTH 20
#define PADDLE_WIDTH 2
#define BALL_RADIUS 3

// Movement
#define MAX_PADDLE_SPEED 8
#define PADDLE_ACCELERATION 2
#define PADDLE_SPEED_BOOST_MULTIPLIER 2

// Collision
#define PADDLE_COLLISION_COOLDOWN_FRAMES 8

// Input
#define JOYSTICK_DEADZONE 10
```

### Adding Features

#### New Game State
1. Add enum to `GameState` in `game_controller.h`
2. Add case to state machine in `game_controller.c::update_game_controller()`
3. Add rendering in `game_controller.c::draw_game_controller()`

#### New Shape Type
1. Add to `ShapeType` enum in `shapes.h`
2. Implement `create_*()` constructor in `shapes.c`
3. Implement `draw_*_impl()` in `shapes.c`

#### Custom Collision Behavior
1. Create callback function matching `CollisionCallback` signature
2. Pass to `init_physics()` when creating object
3. Implement response logic (bounce, destroy, etc.)

### Performance Notes

- **Frame Rate**: ~12 fps (SPI communication bottleneck)
- **Display Update**: ~80ms per frame
- **SPI Clock**: 1.67 MHz maximum (ATtiny1627 peripheral limitation)
- **Physics Update**: <1ms (computational time)
- **Input Polling**: ~0.1ms (ADC read time)

### Memory Usage

- **Flash**: ~8KB (code + constants)
- **SRAM**: ~1KB (game objects + stack)
- **Display Buffer**: 1KB (128×64 / 8 bits)

### Hardware Limitations

This project uses an SH1106 monochrome OLED display due to ATtiny1627 constraints. An attempt was made to migrate to an ST7789 color TFT display, but the ATtiny1627's limited SPI throughput (max 1.67 MHz) and 2KB SRAM proved insufficient for acceptable frame rates with color graphics. For color display applications, consider upgrading to a more capable microcontroller (e.g., ESP32) with DMA-enabled SPI and larger memory.

## 📝 License

[Specify your license here]

## 👥 Authors

- Wes Orr - Initial development

## 🙏 Acknowledgments

- Built with assistance from Claude Code
- SH1106 driver adapted from [source if applicable]

## 📧 Contact

[Your contact information]

---

**Have fun playing PaddlePanic!** 🎮
