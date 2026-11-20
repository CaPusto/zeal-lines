# Match rules (L2 Reborn)

This is the exact specification of the rules implemented in `src/game.c` and
verified by `tests/test_game.c` (see `tests/run_tests.sh`).

## Board and moves

- Board `9×9`, colours `1..7`, `0` is an empty cell.
- A move: select a ball, pick an empty cell, verify an orthogonal path
  (BFS over horizontal/vertical steps), then move the ball.
- Diagonal steps are not allowed.
- After the move, only figures that contain the destination cell are checked.

## Figures

### Line

A continuous run of one colour of length **5 or more** through the moved ball:

- horizontal;
- vertical;
- main diagonal;
- anti-diagonal.

### Direct cross `+`

A full 5-cell figure: the centre and its four orthogonal neighbours.

The centre can be:

- the destination cell itself;
- any of the four orthogonal neighbours of the moved ball.

The moved ball can therefore be either the centre or an arm of the cross.
This matches the GDD trigger "the row contains 3 or 4 balls".

### Diagonal cross `X`

A full 5-cell figure: the centre and its four diagonal neighbours.

The centre can be:

- the destination cell itself;
- any of the four diagonal neighbours of the moved ball.

## Merging and scoring

All cells of all matched figures are merged into a **duplicate-free set**.
A centre cell shared by `+` and `X` is counted once.

```
points per move = 2 × number of unique cells
```

Examples:

| Figure | Unique cells | Points |
|---|---:|---:|
| Line of 5 | 5 | 10 |
| `+` | 5 | 10 |
| `X` | 5 | 10 |
| `+` and `X` together | 9 | 18 |
| Line of 5 plus perpendicular wings | 7 | 14 |
| Line of 4 plus a full `+` | 5 | 10; the 4th line cell stays |

A line of four balls is not cleared by itself.

## Move and spawn

1. The player moves a ball.
2. If at least one figure is found:
   - the balls are removed;
   - points are added;
   - new balls are **not** added;
   - the player makes the next move.
3. If there is no figure:
   - up to 3 preview balls are placed on the board;
   - each placed ball is replaced by a new random colour in the preview;
   - figures formed by the new balls are checked;
   - if the board is full, the game ends.

If fewer than 3 empty cells are available, only as many balls as fit are
placed. If the board is full after that, it is game over.

## Losing

The game ends when no empty cells remain after the move and spawn (all 81 cells
are occupied). With at least one empty cell a move is always possible: a ball
next to it can move there.

## Note on the L2 reference

The JavaScript version of L2 concatenates figure arrays without de-duplication,
so `+` and `X` together score 20 instead of 18, and a line of 5 with wings
scores 20 instead of 14. This implementation merges sets, as the GDD requires.
