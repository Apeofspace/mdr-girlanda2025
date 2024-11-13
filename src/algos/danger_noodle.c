#include "main.h"
#include "helpers.h"

#define _S_TIMEOUT_MS 300
struct snake_errors_t {
  volatile uint32_t food_gen;
} snake_errors = {
  .food_gen = 0,
};

typedef struct {
  int len;
  struct body_pix_t {
    pixel_t pix;
    int pos;
    int dir; //1 or -1
  } body[LEDS_NUMBER];
  struct food_pix_t {
    pixel_t pix;
    int pos;
  } food;
} snake_par_t;
snake_par_t snake1;

int get_new_food_pos(snake_par_t *snake) {
  bool bad = true;
  int pos = 0;
  uint32_t t0 = GetMs();
  while (bad) {
    bad = false;
    pos = (uint32_t)random(0) % LEDS_NUMBER;
    for (int i = 0; i <= snake->len; i++) {
      if (snake->body[i].pos == pos) {
        bad = true;
        break;
      }
    }
    if ((GetMs() - t0) >= _S_TIMEOUT_MS) {
      snake_errors.food_gen++;
      pos = 0;
      break;
    }
  }
  // sanity check
  pos = MAX(0, pos);
  pos = MIN(LEDS_NUMBER - 1, pos);
  return pos;
}

void init_snake(snake_par_t *snake) {
  // draw smol noodle
  // memset(snake, 0, sizeof(snake_par_t)); // auto clean

  // <MANUAL CLEAN>
  for (int i = 0; i < LEDS_NUMBER; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    b->pos = 0;
    b->dir = 1;
    b->pix.red = 0;
    b->pix.green = 0;
    b->pix.blue = 0;
  }
  snake->food.pos = 100;
  snake->food.pix.red = 100;
  snake->food.pix.green = 100;
  snake->food.pix.blue = 100;
  // </MANUAL CLEAN>

  snake->len = 2;
  for (int i = 0; i <= snake->len; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    b->pos = 4 - i;
    b->dir = 1;
    set_random_pixel_color(&(b->pix));
  }
  set_pix_color(&(snake->body[0].pix), 255, 0, 0); // head is red
  // spawn new food
  snake->food.pos = get_new_food_pos(snake);
  set_random_pixel_color(&(snake->food.pix));
}

void snake_baseline(snake_par_t *snake, pixel_t *pix) {
  struct body_pix_t *head = &(snake->body[0]);
  // move body
  for (int i = 0; i <= snake->len; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    int new_pos = b->pos + b->dir;
    b->pos = new_pos;
    if ((new_pos == 0) || (new_pos == (LEDS_NUMBER - 1))) {
      b->dir *= -1; // flip dir when hits the ends
    }
  }
  // eat food
  if (head->pos == snake->food.pos) {
    // expand snake
    struct body_pix_t *tail = &(snake->body[snake->len]);
    snake->len++;
    struct body_pix_t *new_pix = &(snake->body[snake->len]);
    if ((tail->pos == 0) || (tail->pos == LEDS_NUMBER)) {
      // spawned on the pivot point
      new_pix->pos = tail->pos;
      new_pix->dir = tail->dir * -1;
    } else {
      // normal case
      new_pix->pos = tail->pos - tail->dir;
      new_pix->dir = tail->dir;
    }
    copy_pix_color(&(new_pix->pix), &(snake->food.pix));
    // spawn new food
    snake->food.pos = get_new_food_pos(snake);
    set_random_pixel_color(&(snake->food.pix));
  }
  // win
  if (snake->len >= LEDS_NUMBER - 2) {
    // placeholder
    state.recently_switched_algo = true; // force reinit
  }
  // draw everything
  for (int i = snake->len; i >= 0; i--) {
    struct body_pix_t *b = &(snake->body[i]);
    copy_pix_color(&(pix[b->pos]), &(b->pix));
  }
  // draw food
  copy_pix_color(&(pix[snake->food.pos]), &(snake->food.pix));
  // glowing_sides(pix, snake.food.pos, snake.food.pos, 2);
}

void danger_noodle(pixel_t *pix) {
  // init
  if (state.recently_switched_algo) {
    init_snake(&snake1);
  }
  clear_pixels(pix);
  snake_baseline(&snake1, pix);
}
