#define SNAKE_EXPORTS
#include "snake.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  int x;
  int y;
} Pt;

struct SnakeGame {
  int w;
  int h;
  unsigned int rng;

  Pt *snake;     /* [w*h] */
  int snake_len; /* >= 1 */

  int dir_x;
  int dir_y;

  int food_x;
  int food_y;

  int score;
  int alive;

  unsigned char *occ; /* occupancy map [w*h], 1 if snake occupies */
};

static unsigned int xorshift32(unsigned int *state) {
  unsigned int x = *state;
  if (x == 0) x = 2463534242u;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

static int idx_of(SnakeGame *g, int x, int y) { return y * g->w + x; }

static void place_food(SnakeGame *g) {
  int tries = g->w * g->h * 2;
  while (tries-- > 0) {
    int x = (int)(xorshift32(&g->rng) % (unsigned int)g->w);
    int y = (int)(xorshift32(&g->rng) % (unsigned int)g->h);
    if (!g->occ[idx_of(g, x, y)]) {
      g->food_x = x;
      g->food_y = y;
      return;
    }
  }
  /* fallback: scan */
  for (int y = 0; y < g->h; y++) {
    for (int x = 0; x < g->w; x++) {
      if (!g->occ[idx_of(g, x, y)]) {
        g->food_x = x;
        g->food_y = y;
        return;
      }
    }
  }
  /* no place: snake filled the board */
  g->food_x = -1;
  g->food_y = -1;
}

static void init_game(SnakeGame *g, unsigned int seed) {
  g->rng = seed ? seed : 1u;
  g->score = 0;
  g->alive = 1;

  memset(g->occ, 0, (size_t)(g->w * g->h));

  int cx = g->w / 2;
  int cy = g->h / 2;
  g->snake_len = 3;
  g->snake[0] = (Pt){cx, cy};
  g->snake[1] = (Pt){cx - 1, cy};
  g->snake[2] = (Pt){cx - 2, cy};
  for (int i = 0; i < g->snake_len; i++) {
    g->occ[idx_of(g, g->snake[i].x, g->snake[i].y)] = 1;
  }

  g->dir_x = 1;
  g->dir_y = 0;

  place_food(g);
}

SNAKE_API SnakeGame *snake_create(int width, int height, unsigned int seed) {
  if (width < 5) width = 5;
  if (height < 5) height = 5;
  if (width > 80) width = 80;
  if (height > 80) height = 80;

  SnakeGame *g = (SnakeGame *)calloc(1, sizeof(SnakeGame));
  if (!g) return NULL;
  g->w = width;
  g->h = height;
  int cap = width * height;
  g->snake = (Pt *)calloc((size_t)cap, sizeof(Pt));
  g->occ = (unsigned char *)calloc((size_t)cap, 1);
  if (!g->snake || !g->occ) {
    snake_destroy(g);
    return NULL;
  }
  init_game(g, seed);
  return g;
}

SNAKE_API void snake_destroy(SnakeGame *g) {
  if (!g) return;
  free(g->snake);
  free(g->occ);
  free(g);
}

SNAKE_API void snake_reset(SnakeGame *g, unsigned int seed) {
  if (!g) return;
  init_game(g, seed);
}

SNAKE_API void snake_set_dir(SnakeGame *g, int dx, int dy) {
  if (!g) return;
  if ((dx == 0 && dy == 0) || (dx != 0 && dy != 0)) return;
  /* prevent 180-degree reverse */
  if (g->dir_x + dx == 0 && g->dir_y + dy == 0) return;
  g->dir_x = dx;
  g->dir_y = dy;
}

SNAKE_API int snake_step(SnakeGame *g) {
  if (!g) return 0;
  if (!g->alive) return 0;

  Pt head = g->snake[0];
  Pt next = (Pt){head.x + g->dir_x, head.y + g->dir_y};

  if (next.x < 0 || next.x >= g->w || next.y < 0 || next.y >= g->h) {
    g->alive = 0;
    return 0;
  }

  int next_i = idx_of(g, next.x, next.y);
  /* allow moving into the tail cell only if not growing (classic rule); easiest:
     compute tail cell and temporarily ignore it. */
  Pt tail = g->snake[g->snake_len - 1];
  int tail_i = idx_of(g, tail.x, tail.y);
  int will_grow = (next.x == g->food_x && next.y == g->food_y);
  if (g->occ[next_i] && !(next_i == tail_i && !will_grow)) {
    g->alive = 0;
    return 0;
  }

  /* shift body */
  if (will_grow) {
    /* increase length */
    /* snake array has capacity w*h, safe */
    for (int i = g->snake_len; i > 0; i--) g->snake[i] = g->snake[i - 1];
    g->snake[0] = next;
    g->snake_len += 1;
    g->occ[next_i] = 1;
    g->score += 10;
    place_food(g);
  } else {
    /* move: clear tail, shift, set head */
    g->occ[tail_i] = 0;
    for (int i = g->snake_len - 1; i > 0; i--) g->snake[i] = g->snake[i - 1];
    g->snake[0] = next;
    g->occ[next_i] = 1;
  }

  /* win: filled board */
  if (g->snake_len >= g->w * g->h) {
    g->alive = 0;
  }

  return g->alive;
}

SNAKE_API int snake_get_width(SnakeGame *g) { return g ? g->w : 0; }
SNAKE_API int snake_get_height(SnakeGame *g) { return g ? g->h : 0; }
SNAKE_API int snake_get_score(SnakeGame *g) { return g ? g->score : 0; }
SNAKE_API int snake_get_alive(SnakeGame *g) { return g ? g->alive : 0; }
SNAKE_API int snake_get_food_x(SnakeGame *g) { return g ? g->food_x : -1; }
SNAKE_API int snake_get_food_y(SnakeGame *g) { return g ? g->food_y : -1; }
SNAKE_API int snake_get_snake_len(SnakeGame *g) { return g ? g->snake_len : 0; }

SNAKE_API void snake_get_snake_xy(SnakeGame *g, int idx, int *out_x, int *out_y) {
  if (!g || idx < 0 || idx >= g->snake_len) {
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    return;
  }
  if (out_x) *out_x = g->snake[idx].x;
  if (out_y) *out_y = g->snake[idx].y;
}

