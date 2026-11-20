#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "game.h"

/* Deterministic host-side replacement for the ZGDK RNG. */
static uint16_t rng = 0x1234;

char rand8(void)
{
    rng = (uint16_t)(rng * 1103515245u + 12345u);
    return (char)(rng >> 8);
}

void rand8_seed(uint16_t seed)
{
    if (seed)
        rng = seed;
}

char rand8_quick(void)
{
    return 0x42;
}


static void reset(game_t* g)
{
    memset(g, 0, sizeof(*g));
    g->score = 0;
}


static void put(game_t* g, int x, int y, int c)
{
    g->cells[y][x] = (uint8_t)c;
}


static int count_balls(const game_t* g)
{
    int n = 0;
    int x;
    int y;

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (g->cells[y][x])
                n++;
        }
    }

    return n;
}


int main(void)
{
    game_t g;
    uint8_t n;
    int x;
    int y;

    /* A direct cross of five balls. */
    reset(&g);
    put(&g, 4, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 4, 3, 1);
    put(&g, 4, 5, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("plus: n=%u score=%u balls=%d\n", n, g.score, count_balls(&g));
    assert(n == 5);
    assert(g.score == 10);
    assert(count_balls(&g) == 0);

    /* The moved ball may be an arm of the cross rather than its centre. */
    reset(&g);
    put(&g, 4, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 4, 3, 1);
    put(&g, 4, 5, 1);
    n = game_resolve_matches(&g, 5, 4);
    printf("plus arm: n=%u score=%u\n", n, g.score);
    assert(n == 5);
    assert(g.score == 10);

    /* A cross and an X share their centre: 9 unique balls, 18 points. */
    reset(&g);
    put(&g, 4, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 4, 3, 1);
    put(&g, 4, 5, 1);
    put(&g, 3, 3, 1);
    put(&g, 5, 5, 1);
    put(&g, 3, 5, 1);
    put(&g, 5, 3, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("plus+X: n=%u score=%u\n", n, g.score);
    assert(n == 9);
    assert(g.score == 18);

    /* A straight line of five. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 6, 4, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("line5: n=%u score=%u\n", n, g.score);
    assert(n == 5);
    assert(g.score == 10);

    /* A line of five with perpendicular wings: 7 unique balls, 14 points. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 6, 4, 1);
    put(&g, 4, 3, 1);
    put(&g, 4, 5, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("line5+wings: n=%u score=%u\n", n, g.score);
    assert(n == 7);
    assert(g.score == 14);

    /* A line of four plus a cross: the extra line cell is not removed. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 4, 3, 1);
    put(&g, 4, 5, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("line4+plus: n=%u score=%u extra=%u\n",
           n, g.score, g.cells[4][2]);
    assert(n == 5);
    assert(g.score == 10);
    assert(g.cells[4][2] == 1);

    /* A line of four alone is not a match. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("line4: n=%u score=%u\n", n, g.score);
    assert(n == 0);
    assert(g.score == 0);
    assert(count_balls(&g) == 4);

    /* An X of five. */
    reset(&g);
    put(&g, 4, 4, 1);
    put(&g, 3, 3, 1);
    put(&g, 5, 5, 1);
    put(&g, 3, 5, 1);
    put(&g, 5, 3, 1);
    n = game_resolve_matches(&g, 4, 4);
    printf("X5: n=%u score=%u\n", n, g.score);
    assert(n == 5);
    assert(g.score == 10);

    /* Pathfinding. */
    reset(&g);
    put(&g, 0, 0, 1);
    printf("path adjacent: %u\n", game_find_path(&g, 0, 0, 1, 0));
    assert(game_find_path(&g, 0, 0, 1, 0) == 1);
    assert(game_find_path(&g, 0, 0, 0, 1) == 1);
    assert(game_find_path(&g, 0, 0, 0, 0) == 0);
    printf("path far: %u\n", game_find_path(&g, 0, 0, 8, 8));
    assert(game_find_path(&g, 0, 0, 8, 8) == 1);

    reset(&g);
    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (!(x == 2 && y == 2))
                put(&g, x, y, 1);
        }
    }
    printf("path blocked: %u\n", game_find_path(&g, 0, 0, 2, 2));
    assert(game_find_path(&g, 0, 0, 2, 2) == 0);

    /* A move that completes a line: five removed, no new balls. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 0, 4, 1);
    g.preview[0] = 2;
    g.preview[1] = 3;
    g.preview[2] = 4;
    printf("move+match valid=%u\n", game_try_move(&g, 0, 4, 1, 4));
    printf("move+match: balls=%d score=%u preview=%u\n",
           count_balls(&g), g.score, g.preview[0]);
    assert(g.score == 10);
    assert(count_balls(&g) == 0);
    assert(g.preview[0] == 2);

    /* A move without a match: three preview balls are spawned. */
    reset(&g);
    put(&g, 0, 0, 1);
    g.preview[0] = 2;
    g.preview[1] = 3;
    g.preview[2] = 4;
    printf("move plain valid=%u\n", game_try_move(&g, 0, 0, 1, 0));
    printf("move plain: balls=%d score=%u preview=%u,%u,%u\n",
           count_balls(&g), g.score,
           g.preview[0], g.preview[1], g.preview[2]);
    assert(count_balls(&g) == 4);
    assert(g.score == 0);

    /* An invalid move must not change the board. */
    reset(&g);
    put(&g, 0, 0, 1);
    put(&g, 1, 0, 2);
    assert(game_try_move(&g, 0, 0, 1, 0) == 0);
    assert(g.cells[0][0] == 1);
    assert(g.cells[0][1] == 2);

    /* Explicit path output. */
    reset(&g);
    put(&g, 0, 0, 1);
    {
        uint8_t path[GRID_SIZE * GRID_SIZE];
        uint8_t len;
        uint8_t i;
        uint8_t prev = 0;
        assert(game_path(&g, 0, 0, 8, 8, path, &len) == 1);
        printf("path explicit: len=%u\n", len);
        assert(len == 16);
        assert(path[len - 1] == 8 * GRID_SIZE + 8);
        for (i = 0; i < len; i++) {
            uint8_t px = path[i] % GRID_SIZE;
            uint8_t py = path[i] / GRID_SIZE;
            uint8_t qx = prev % GRID_SIZE;
            uint8_t qy = prev / GRID_SIZE;
            uint8_t dx = px > qx ? px - qx : qx - px;
            uint8_t dy = py > qy ? py - qy : qy - py;
            assert(dx + dy == 1);
            assert(g.cells[py][px] == 0);
            prev = path[i];
        }
    }

    /* Match preview must remove the source ball before simulating the move. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 0, 4, 1);
    {
        uint8_t mark[GRID_SIZE * GRID_SIZE];
        uint8_t count = game_match_preview(&g, 0, 4, 1, 1, 4, mark);
        printf("match preview: count=%u source=%u target=%u\n",
               count, mark[4 * GRID_SIZE + 0],
               mark[4 * GRID_SIZE + 1]);
        assert(count == 5);
        assert(mark[4 * GRID_SIZE + 0] == 0);
        assert(mark[4 * GRID_SIZE + 1] == 1);
    }

    /* Remove a mark and score it once. */
    reset(&g);
    put(&g, 2, 4, 1);
    put(&g, 3, 4, 1);
    put(&g, 4, 4, 1);
    put(&g, 5, 4, 1);
    put(&g, 6, 4, 1);
    {
        uint8_t mark[GRID_SIZE * GRID_SIZE];
        uint8_t count = game_match_mark(&g, 4, 4, mark);
        assert(count == 5);
        count = game_remove_mark(&g, mark);
        printf("remove mark: count=%u score=%u balls=%d\n",
               count, g.score, count_balls(&g));
        assert(count == 5);
        assert(g.score == 10);
        assert(count_balls(&g) == 0);
    }

    /* Preview placement without immediate match resolution. */
    reset(&g);
    g.preview[0] = 1;
    g.preview[1] = 2;
    g.preview[2] = 3;
    {
        uint8_t sx[PREVIEW_COUNT];
        uint8_t sy[PREVIEW_COUNT];
        uint8_t n;
        game_place_preview(&g, sx, sy, &n);
        printf("place preview: n=%u balls=%d score=%u\n",
               n, count_balls(&g), g.score);
        assert(n == 3);
        assert(count_balls(&g) == 3);
        assert(g.score == 0);
    }

    /* A fresh game starts with exactly three balls and a preview. */
    reset(&g);
    game_init(&g);
    printf("init: balls=%d score=%u over=%u preview=%u,%u,%u\n",
           count_balls(&g), g.score, g.game_over,
           g.preview[0], g.preview[1], g.preview[2]);
    assert(count_balls(&g) == 3);
    assert(g.score == 0);
    assert(g.game_over == 0);

    /* Spawning on an empty board adds three balls. */
    reset(&g);
    g.preview[0] = 1;
    g.preview[1] = 2;
    g.preview[2] = 3;
    game_spawn_preview(&g);
    printf("spawn3: balls=%d over=%u\n", count_balls(&g), g.game_over);
    assert(count_balls(&g) == 3);
    assert(g.game_over == 0);

    /* Spawning into the last free cell fills the board. */
    reset(&g);
    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (!(x == 8 && y == 8))
                put(&g, x, y, 1);
        }
    }
    g.preview[0] = 2;
    g.preview[1] = 3;
    g.preview[2] = 4;
    game_spawn_preview(&g);
    printf("spawn1: balls=%d over=%u\n", count_balls(&g), g.game_over);
    assert(count_balls(&g) == 81);
    assert(g.game_over == 1);

    /* Empty-board detection used to avoid a dead end after a clear. */
    reset(&g);
    assert(game_has_balls(&g) == 0);
    put(&g, 0, 0, 1);
    assert(game_has_balls(&g) == 1);

    printf("all game tests passed\n");
    return 0;
}
