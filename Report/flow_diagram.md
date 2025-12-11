# PaddlePanic Flow Diagram Nodes

## Main Flow

1. **START** (oval)

2. **Initialize Hardware** (rectangle)
   - SPI, ADC, GPIO

3. **Initialize Game Controller** (rectangle)
   - Create walls, paddles, ball
   - Set state to TITLE

4. **Main Loop** (diamond - loop condition)

---

## Inside Main Loop

5. **Update Input Controller** (rectangle)
   - Poll joystick ADC
   - Poll buttons

6. **Check Game State** (diamond)
   - TITLE?
   - BALL_AT_REST?
   - BALL_MOVING?
   - PAUSED?
   - COUNTDOWN?
   - GAME_OVER?

---

## State: TITLE

7. **Display Title Screen** (rectangle)

8. **Button Pressed?** (diamond)

9. **Reset Game** (rectangle)
   - Score = 0
   - Center paddles & ball

10. **Transition to BALL_AT_REST** (rectangle)

---

## State: BALL_AT_REST

11. **Update Paddle Positions** (rectangle)
    - Normalize joystick input
    - Apply piecewise-linear mapping
    - Smooth acceleration
    - Clamp to bounds

12. **Button Pressed?** (diamond)

13. **Generate Random Direction** (rectangle)
    - Sample ADC for seed
    - Select velocity vector

14. **Transition to BALL_MOVING** (rectangle)

---

## State: BALL_MOVING

15. **Update Paddle Positions** (rectangle)
    - (Same as BALL_AT_REST)

16. **Update Ball Position** (rectangle)
    - Apply velocity

17. **Check Paddle Collisions** (diamond - loop for 4 paddles)

18. **Collision Detected & Cooldown = 0?** (diamond)

19. **Handle Paddle Hit** (rectangle)
    - Increment score
    - Reverse ball velocity
    - Set cooldown = 8

20. **Check Wall Collisions** (diamond - loop for 4 walls)

21. **Wall Collision?** (diamond)

22. **Handle Game Over** (rectangle)
    - Invert display (flash)
    - Save final score
    - Stop ball

23. **Transition to GAME_OVER** (rectangle)

24. **Button Pressed?** (diamond)

25. **Save Ball Velocity** (rectangle)

26. **Transition to PAUSED** (rectangle)

27. **Decrement Cooldowns** (rectangle)
    - All paddles with cooldown > 0

---

## State: PAUSED

28. **Display Pause Menu** (rectangle)
    - Show score

29. **Button Pressed?** (diamond)

30. **Set Countdown Timer = 36** (rectangle)

31. **Transition to COUNTDOWN** (rectangle)

---

## State: COUNTDOWN

32. **Decrement Timer** (rectangle)

33. **Display Countdown Number** (rectangle)
    - 3, 2, or 1 based on timer

34. **Timer = 0?** (diamond)

35. **Restore Ball Velocity** (rectangle)

36. **Transition to BALL_MOVING** (rectangle)

---

## State: GAME_OVER

37. **Display Final Score** (rectangle)

38. **Button Pressed?** (diamond)

39. **Transition to TITLE** (rectangle)

---

## Rendering (Common to All States)

40. **Clear Framebuffer** (rectangle)

41. **Draw Game State** (rectangle)
    - State-specific rendering

42. **Refresh Display** (rectangle)
    - SPI transfer to hardware

43. **Loop Back to Main Loop** (arrow back to step 4)

---

## Node Type Summary

- **Ovals**: START/END points
- **Rectangles**: Process/action steps
- **Diamonds**: Decision points (conditionals)
- **Arrows**: Flow direction with state labels