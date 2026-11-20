#include <stdint.h>
#include <string.h>

#include <zgdk/utils.h>

#include "game.h"

/*
 * The board is checked with a mark buffer so that a line and a cross that
 * share cells are removed once and scored once.
 */
static uint8_t match_mark[GRID_SIZE][GRID_SIZE];

static uint8_t path_dist[GRID_SIZE][GRID_SIZE];
static uint8_t path_queue[GRID_SIZE * GRID_SIZE];
static uint8_t path_prev[GRID_SIZE * GRID_SIZE];


static uint8_t cell_color(const game_t* game, int8_t x, int8_t y, uint8_t color)
{
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE)
        return 0;

    return game->cells[(uint8_t)y][(uint8_t)x] == color;
}


static void mark_cell(int8_t x, int8_t y)
{
    match_mark[(uint8_t)y][(uint8_t)x] = 1;
}


static uint8_t count_empty(const game_t* game)
{
    uint8_t x;
    uint8_t y;
    uint8_t count = 0;

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (game->cells[y][x] == 0)
                count++;
        }
    }

    return count;
}


static uint8_t nth_empty(const game_t* game, uint8_t n,
                         uint8_t* x, uint8_t* y)
{
    uint8_t cx;
    uint8_t cy;

    for (cy = 0; cy < GRID_SIZE; cy++) {
        for (cx = 0; cx < GRID_SIZE; cx++) {
            if (game->cells[cy][cx] == 0) {
                if (n == 0) {
                    *x = cx;
                    *y = cy;
                    return 1;
                }
                n--;
            }
        }
    }

    return 0;
}


uint8_t game_random_color(void)
{
    uint8_t value;

    /* Reject the tail so all seven colors are equally likely. */
    do {
        value = (uint8_t)rand8();
    } while (value >= 252);

    return (uint8_t)(1 + (value % COLOR_COUNT));
}


uint8_t game_has_ball(const game_t* game, uint8_t x, uint8_t y)
{
    if (x >= GRID_SIZE || y >= GRID_SIZE)
        return 0;

    return game->cells[y][x] != 0;
}


uint8_t game_board_full(const game_t* game)
{
    return count_empty(game) == 0;
}


uint8_t game_has_balls(const game_t* game)
{
    uint8_t x;
    uint8_t y;

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (game->cells[y][x] != 0)
                return 1;
        }
    }

    return 0;
}


uint8_t game_path(const game_t* game, uint8_t sx, uint8_t sy,
                  uint8_t tx, uint8_t ty,
                  uint8_t* path, uint8_t* path_len)
{
    uint8_t head;
    uint8_t tail;
    uint8_t idx;
    uint8_t x;
    uint8_t y;
    uint8_t i;
    uint8_t n;
    uint8_t cur;
    uint8_t start;

    if (sx >= GRID_SIZE || sy >= GRID_SIZE ||
        tx >= GRID_SIZE || ty >= GRID_SIZE)
        return 0;

    if (game->cells[sy][sx] == 0 || game->cells[ty][tx] != 0)
        return 0;

    if (sx == tx && sy == ty)
        return 0;

    memset(path_dist, 0xFF, sizeof(path_dist));

    start = (uint8_t)(sy * GRID_SIZE + sx);
    head = 0;
    tail = 0;
    path_dist[sy][sx] = 0;
    path_queue[tail++] = start;

    while (head < tail) {
        idx = path_queue[head++];
        x = (uint8_t)(idx % GRID_SIZE);
        y = (uint8_t)(idx / GRID_SIZE);

        if (x == tx && y == ty) {
            n = 0;
            cur = (uint8_t)(ty * GRID_SIZE + tx);

            while (cur != start) {
                if (path != NULL)
                    path[n] = cur;
                n++;
                cur = path_prev[cur];
            }

            if (path != NULL) {
                for (i = 0; i < (uint8_t)(n / 2); i++) {
                    uint8_t tmp = path[i];
                    path[i] = path[n - 1 - i];
                    path[n - 1 - i] = tmp;
                }
            }

            if (path_len != NULL)
                *path_len = n;

            return 1;
        }

        if (x > 0 &&
            game->cells[y][x - 1] == 0 &&
            path_dist[y][x - 1] == 0xFF) {
            path_dist[y][x - 1] = (uint8_t)(path_dist[y][x] + 1);
            path_prev[y * GRID_SIZE + x - 1] = idx;
            path_queue[tail++] = (uint8_t)(y * GRID_SIZE + x - 1);
        }
        if (x + 1 < GRID_SIZE &&
            game->cells[y][x + 1] == 0 &&
            path_dist[y][x + 1] == 0xFF) {
            path_dist[y][x + 1] = (uint8_t)(path_dist[y][x] + 1);
            path_prev[y * GRID_SIZE + x + 1] = idx;
            path_queue[tail++] = (uint8_t)(y * GRID_SIZE + x + 1);
        }
        if (y > 0 &&
            game->cells[y - 1][x] == 0 &&
            path_dist[y - 1][x] == 0xFF) {
            path_dist[y - 1][x] = (uint8_t)(path_dist[y][x] + 1);
            path_prev[(y - 1) * GRID_SIZE + x] = idx;
            path_queue[tail++] = (uint8_t)((y - 1) * GRID_SIZE + x);
        }
        if (y + 1 < GRID_SIZE &&
            game->cells[y + 1][x] == 0 &&
            path_dist[y + 1][x] == 0xFF) {
            path_dist[y + 1][x] = (uint8_t)(path_dist[y][x] + 1);
            path_prev[(y + 1) * GRID_SIZE + x] = idx;
            path_queue[tail++] = (uint8_t)((y + 1) * GRID_SIZE + x);
        }
    }

    return 0;
}


uint8_t game_find_path(const game_t* game, uint8_t sx, uint8_t sy,
                       uint8_t tx, uint8_t ty)
{
    return game_path(game, sx, sy, tx, ty, NULL, NULL);
}


uint8_t game_match_mark(const game_t* game, uint8_t x, uint8_t y,
                        uint8_t* mark)
{
    static const int8_t dirs[4][2] = {
        { 1,  0 },
        { 0,  1 },
        { 1,  1 },
        { 1, -1 }
    };
    static const int8_t plus_centers[5][2] = {
        {  0,  0 },
        {  1,  0 },
        { -1,  0 },
        {  0,  1 },
        {  0, -1 }
    };
    static const int8_t cross_centers[5][2] = {
        {  0,  0 },
        {  1,  1 },
        {  1, -1 },
        { -1,  1 },
        { -1, -1 }
    };

    uint8_t color;
    uint8_t removed;
    uint8_t i;
    uint8_t d;
    uint8_t count;
    uint8_t step;
    int8_t dx;
    int8_t dy;
    int8_t nx;
    int8_t ny;
    int8_t cx;
    int8_t cy;
    uint8_t mx;
    uint8_t my;

    if (mark == NULL)
        return 0;

    color = game->cells[y][x];

    if (color == 0) {
        memset(mark, 0, GRID_SIZE * GRID_SIZE);
        return 0;
    }

    memset(match_mark, 0, sizeof(match_mark));

    /* Straight lines through the ball. */
    for (d = 0; d < 4; d++) {
        dx = dirs[d][0];
        dy = dirs[d][1];
        count = 1;

        for (i = 0; i < 2; i++) {
            int8_t direction = (i == 0) ? -1 : 1;

            for (step = 1; ; step++) {
                nx = (int8_t)x + dx * (int8_t)step * direction;
                ny = (int8_t)y + dy * (int8_t)step * direction;

                if (!cell_color(game, nx, ny, color))
                    break;

                count++;
            }
        }

        if (count >= 5) {
            mark_cell((int8_t)x, (int8_t)y);

            for (i = 0; i < 2; i++) {
                int8_t direction = (i == 0) ? -1 : 1;

                for (step = 1; ; step++) {
                    nx = (int8_t)x + dx * (int8_t)step * direction;
                    ny = (int8_t)y + dy * (int8_t)step * direction;

                    if (!cell_color(game, nx, ny, color))
                        break;

                    mark_cell(nx, ny);
                }
            }
        }
    }

    /* Direct crosses. */
    for (i = 0; i < 5; i++) {
        cx = (int8_t)x + plus_centers[i][0];
        cy = (int8_t)y + plus_centers[i][1];

        if (!cell_color(game, cx, cy, color))
            continue;

        if (cell_color(game, (int8_t)(cx + 1), cy, color) &&
            cell_color(game, (int8_t)(cx - 1), cy, color) &&
            cell_color(game, cx, (int8_t)(cy + 1), color) &&
            cell_color(game, cx, (int8_t)(cy - 1), color)) {
            mark_cell(cx, cy);
            mark_cell((int8_t)(cx + 1), cy);
            mark_cell((int8_t)(cx - 1), cy);
            mark_cell(cx, (int8_t)(cy + 1));
            mark_cell(cx, (int8_t)(cy - 1));
        }
    }

    /* Diagonal crosses. */
    for (i = 0; i < 5; i++) {
        cx = (int8_t)x + cross_centers[i][0];
        cy = (int8_t)y + cross_centers[i][1];

        if (!cell_color(game, cx, cy, color))
            continue;

        if (cell_color(game, (int8_t)(cx + 1), (int8_t)(cy + 1), color) &&
            cell_color(game, (int8_t)(cx - 1), (int8_t)(cy - 1), color) &&
            cell_color(game, (int8_t)(cx + 1), (int8_t)(cy - 1), color) &&
            cell_color(game, (int8_t)(cx - 1), (int8_t)(cy + 1), color)) {
            mark_cell(cx, cy);
            mark_cell((int8_t)(cx + 1), (int8_t)(cy + 1));
            mark_cell((int8_t)(cx - 1), (int8_t)(cy - 1));
            mark_cell((int8_t)(cx + 1), (int8_t)(cy - 1));
            mark_cell((int8_t)(cx - 1), (int8_t)(cy + 1));
        }
    }

    removed = 0;

    for (my = 0; my < GRID_SIZE; my++) {
        for (mx = 0; mx < GRID_SIZE; mx++) {
            uint8_t value = match_mark[my][mx] ? 1 : 0;
            mark[my * GRID_SIZE + mx] = value;
            if (value)
                removed++;
        }
    }

    return removed;
}


uint8_t game_remove_mark(game_t* game, const uint8_t* mark)
{
    uint8_t x;
    uint8_t y;
    uint8_t removed = 0;

    if (mark == NULL)
        return 0;

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (mark[y * GRID_SIZE + x] && game->cells[y][x] != 0) {
                game->cells[y][x] = 0;
                removed++;
            }
        }
    }

    if (removed != 0)
        game->score += (uint16_t)removed * 2;

    return removed;
}


uint8_t game_match_preview(const game_t* game, uint8_t sx, uint8_t sy,
                           uint8_t color, uint8_t tx, uint8_t ty,
                           uint8_t* mark)
{
    game_t temp;

    temp = *game;
    temp.cells[sy][sx] = 0;
    temp.cells[ty][tx] = color;

    return game_match_mark(&temp, tx, ty, mark);
}


uint8_t game_resolve_matches(game_t* game, uint8_t x, uint8_t y)
{
    uint8_t mark[GRID_SIZE * GRID_SIZE];
    uint8_t count;

    count = game_match_mark(game, x, y, mark);

    if (count < 5)
        return 0;

    return game_remove_mark(game, mark);
}


void game_move_ball(game_t* game, uint8_t sx, uint8_t sy,
                    uint8_t tx, uint8_t ty)
{
    uint8_t color;

    color = game->cells[sy][sx];
    game->cells[sy][sx] = 0;
    game->cells[ty][tx] = color;
}


void game_place_preview(game_t* game,
                        uint8_t* spawned_x, uint8_t* spawned_y,
                        uint8_t* spawned_count)
{
    uint8_t i;
    uint8_t empty;
    uint8_t pick;
    uint8_t spawned;
    uint16_t limit;

    spawned = 0;

    for (i = 0; i < PREVIEW_COUNT; i++) {
        empty = count_empty(game);

        if (empty == 0)
            break;

        /* Reject values above the largest multiple of empty. */
        limit = (uint16_t)(256 - (256 % empty));
        do {
            pick = (uint8_t)rand8();
        } while (pick >= limit);

        pick = (uint8_t)(pick % empty);

        if (!nth_empty(game, pick, &spawned_x[spawned], &spawned_y[spawned]))
            break;

        game->cells[spawned_y[spawned]][spawned_x[spawned]] = game->preview[i];
        game->preview[i] = game_random_color();
        spawned++;
    }

    *spawned_count = spawned;
}


void game_spawn_preview(game_t* game)
{
    uint8_t spawned_x[PREVIEW_COUNT];
    uint8_t spawned_y[PREVIEW_COUNT];
    uint8_t spawned;
    uint8_t i;

    game_place_preview(game, spawned_x, spawned_y, &spawned);

    /* New balls can complete lines themselves. */
    for (i = 0; i < spawned; i++) {
        if (game->cells[spawned_y[i]][spawned_x[i]] != 0)
            game_resolve_matches(game, spawned_x[i], spawned_y[i]);
    }

    game->game_over = game_board_full(game);
}


uint8_t game_try_move(game_t* game, uint8_t sx, uint8_t sy,
                      uint8_t tx, uint8_t ty)
{
    if (!game_find_path(game, sx, sy, tx, ty))
        return 0;

    game_move_ball(game, sx, sy, tx, ty);

    if (game_resolve_matches(game, tx, ty) == 0) {
        /* No match: the turn ends and three new balls enter the board. */
        game_spawn_preview(game);
    }

    return 1;
}


void game_init(game_t* game)
{
    uint8_t i;

    memset(game->cells, 0, sizeof(game->cells));
    game->score = 0;
    game->game_over = 0;

    for (i = 0; i < PREVIEW_COUNT; i++) {
        game->preview[i] = game_random_color();
    }

    /* The first turn starts with three balls, like the original game. */
    game_spawn_preview(game);
}
