#include "main.h"
#include "helpers.h"

#define _S_TIMEOUT_MS 300
struct snake_errors_t {
  volatile uint32_t food_gen;
  volatile uint32_t init_params_error;
} snake_errors = {
  .food_gen = 0,
  .init_params_error = 0,
};

typedef enum {FORWARD = 1, BACKWARD = -1} snake_dir_t;

typedef struct {
  struct sn_properties_t {
    int max_len;
    int start_pos;
    snake_dir_t start_dir;
  } properties;
  int len;
  struct body_pix_t {
    pixel_t pix;
    int pos;
    snake_dir_t dir; //1 or -1
  } body[LEDS_NUMBER];
  struct food_pix_t {
    pixel_t pix;
    int pos;
  } food;
} snake_par_t;

volatile bool _sn_reinit_required = false;

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

void init_snake(snake_par_t *snake, int max_len, int start_pos, int start_dir) {
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

  // asserts (not extensive really)
  if (!(((start_dir == -1) || (start_dir == 1)) && ((start_pos > 0) && (start_pos <= LEDS_NUMBER)))) {
    snake_errors.init_params_error++;
    state.flags.paused = true;
    return;
  }

  snake->properties.start_dir = start_dir;
  snake->properties.start_pos = start_pos;
  snake->properties.max_len = max_len;
  snake->len = 2;
  for (int i = 0; i <= snake->len; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    b->dir = snake->properties.start_dir;
    b->pos = snake->properties.start_pos - i * b->dir;
    // sanity check
    b->pos = MAX(0, b->pos);
    b->pos = MIN(LEDS_NUMBER - 1, b->pos);
    set_random_pixel_color(&(b->pix));
  }
  set_pix_color(&(snake->body[0].pix), 255, 0, 0); // head is red
  // spawn new food
  snake->food.pos = get_new_food_pos(snake);
  set_random_pixel_color(&(snake->food.pix));
  _sn_reinit_required = false;
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
    if ((tail->pos == 0) || (tail->pos == (LEDS_NUMBER - 1))) {
      // spawned on the pivot point
      new_pix->pos = tail->pos;
      new_pix->dir = tail->dir * -1;
    } else {
      // normal case
      new_pix->pos = tail->pos - tail->dir;
      new_pix->dir = tail->dir;
    }
    // sanity check
    new_pix->pos = MAX(0, new_pix->pos);
    new_pix->pos = MIN(LEDS_NUMBER - 1, new_pix->pos);
    copy_pix_color(&(new_pix->pix), &(snake->food.pix));
    // spawn new food
    snake->food.pos = get_new_food_pos(snake);
    set_random_pixel_color(&(snake->food.pix));
  }
  // win
  if (snake->len >= (snake->properties.max_len)) {
    // placeholder
    _sn_reinit_required = true;
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
  static snake_par_t snake1;
  if (state.recently_switched_algo || _sn_reinit_required) {
    init_snake(&snake1, LEDS_NUMBER - 1, 3, 1);
  }
  clear_pixels(pix);
  snake_baseline(&snake1, pix);
}
