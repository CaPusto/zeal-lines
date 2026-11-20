# Game design document (GDD): L2 Reborn

**L2 Reborn** is a modern remake of the classic Color Lines puzzle, based on the
unique mechanic of the original `CaPusto/L2` project (clearing not only lines but
also compact geometric crosses).

---

## 1. Overview

### 1.1. Concept

A classic casual puzzle on a 9×9 board. The player moves coloured balls to clear
the field. The main difference from standard Lines is the ability to clear balls
not only in a row but also in **compact direct (`+`) and diagonal (`X`) crosses**.

### 1.2. Unique selling point (USP)

- **Deeper geometry:** unlike classic variants, it pays off to plan moves for
  crossed figures (`+` and `X`) made of only 5 balls, which completely changes
  the tactics of avoiding a full board.
- **Combo potential:** lines and crosses can be combined into large monochrome
  patterns for a huge score in a single move.

### 1.3. Audience and platforms

- **Platforms:** web (HTML5 / mobile first), mobile (iOS, Android).
- **Audience:** fans of calm logic games and of classic Color Lines (25–50+).

---

## 2. Gameplay and mechanics

### 2.1. Board and entities

- **Grid:** a square of **9×9 cells**.
- **Ball palette:** 7 base colours (`Yellow`, `Green`, `Red`, `Blue`, `Violet`,
  `Aqua`, `Pink`).
- **Ball states:**
  - *Preview:* shown in the "Next" panel (3 balls).
  - *Idle on the board:* waiting to be selected.
  - *Selected:* animated ball (pulses).
  - *Movement animation:* step-by-step travel across cells.

### 2.2. Core loop

1. The player selects a ball and picks an empty cell.
2. **Pathfinding:** the game checks for a free path (strictly horizontal or
   vertical; diagonal steps are blocked).
3. If a path exists, the ball moves to the destination.
4. **Check match:** the game scans around the moved ball for match conditions.
5. **Branch:**
   - *Match found:* the balls are removed. New balls are **not** added. The
     player makes the next move.
   - *No match:* **3 new random balls** from the preview are placed on the
     board, and 3 new colours are generated for the preview.
6. **Lose condition:** the board is completely full (81 cells) and no move is
   possible.

---

## 3. Exact match conditions

The check runs for the cell where the ball landed. Balls are cleared when the
merged group of one colour is **≥ 5 balls**.

### 3.1. Linear match

- **Geometry:** a continuous run of **5 or more** same-colour balls.
- **Directions:** horizontal, vertical, main diagonal, anti-diagonal.

### 3.2. Direct cross / plus

- **Trigger:** a horizontal or vertical row through the target cell contains
  **3 or 4 balls**.
- **Geometry:** the centre ball and its four orthogonal neighbours:
  `[x+1, y]`, `[x-1, y]`, `[x, y-1]`, `[x, y+1]`.
- **Result:** when all 5 cells are filled, the `+` figure disappears.

### 3.3. Diagonal cross / X

- **Trigger:** a diagonal row contains **3 or 4 balls**.
- **Geometry:** the centre ball and its four diagonal neighbours:
  `[x+1, y+1]`, `[x-1, y-1]`, `[x+1, y-1]`, `[x-1, y+1]`.
- **Result:** when all 5 cells are filled, the `X` figure disappears.

### 3.4. Merging and scoring

- When several figures are completed in one move (for example a line crossing a
  cross), the arrays of removed balls are **merged (concat)** without
  duplicating the centre ball.
- **Score formula:** `each removed ball = 2 points`.
  *Minimum per figure (5 balls):* **10 points**.
  *Double cross (`+` and `X` together = 9 balls):* **18 points**.

---

## 4. Interface and user experience (UI/UX)

### 4.1. Screens

1. **Main menu:** buttons "New game", "High scores", "Settings"
   (sound/music).
2. **Game screen:**
   - central area: the 9×9 board;
   - top panel: current score and high score;
   - bottom/side panel: the "Next" section (3 mini-balls of future colours).

### 4.2. UX/UI features (remake improvements)

- **Visual path:** highlight the trajectory of the ball when hovering an empty
  cell.
- **Match pre-highlight:** if the player moves a ball over a cell that would
  complete a cross or a line, those balls blink slightly to signal the
  upcoming clear.

---

## 5. Technical implementation and architecture

- **Engine:** pure JavaScript (ES6+) Canvas/WebGL (or Pixi.js) for the web
  version, Unity for the mobile port.
- **Save/load:** serialize the board to a string (the original format stores
  `coordinates + colour`, separated by `-`) and write it to LocalStorage.
- **Pathfinding:** a modified Lee algorithm (wave/BFS).

---

## 6. Audio and visual effects (VFX)

- **VFX:**
  - a smooth scale-in for new balls (0 to 1);
  - an "explosion/shrink" effect when balls are cleared (particles in the
    colour of the removed figure).
- **Audio:**
  - a soft click when a ball is selected;
  - a "whistling" movement sound;
  - a juicy, satisfying collapse sound when a combination is destroyed (the
    pitch rises when more than 6 balls are cleared).
