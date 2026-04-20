#pragma once

#ifdef _WIN32
  #ifdef SNAKE_EXPORTS
    #define SNAKE_API __declspec(dllexport)
  #else
    #define SNAKE_API __declspec(dllimport)
  #endif
#else
  #define SNAKE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SnakeGame SnakeGame;

SNAKE_API SnakeGame *snake_create(int width, int height, unsigned int seed);
SNAKE_API void snake_destroy(SnakeGame *g);

SNAKE_API void snake_reset(SnakeGame *g, unsigned int seed);
SNAKE_API void snake_set_dir(SnakeGame *g, int dx, int dy);
SNAKE_API int snake_step(SnakeGame *g); /* returns 1 if alive else 0 */

SNAKE_API int snake_get_width(SnakeGame *g);
SNAKE_API int snake_get_height(SnakeGame *g);
SNAKE_API int snake_get_score(SnakeGame *g);
SNAKE_API int snake_get_alive(SnakeGame *g);

SNAKE_API int snake_get_food_x(SnakeGame *g);
SNAKE_API int snake_get_food_y(SnakeGame *g);

SNAKE_API int snake_get_snake_len(SnakeGame *g);
SNAKE_API void snake_get_snake_xy(SnakeGame *g, int idx, int *out_x, int *out_y);

#ifdef __cplusplus
}
#endif

