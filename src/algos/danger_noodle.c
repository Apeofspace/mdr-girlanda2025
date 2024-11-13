#include "main.h"
#include "helpers.h"

#define _S_TIMEOUT_MS 300
struct snake_errors_t {
  volatile uint32_t food_gen;
  volatile uint32_t init_params_error;
  volatile uint32_t init_wrong_position;
  volatile uint32_t wrong_position;
} snake_errors = {
  .food_gen = 0,
  .init_params_error = 0,
};

typedef enum {FORWARD = 1, BACKWARD = -1} snake_dir_t;

typedef struct {
  struct sn_properties_t {
    int max_len;
    int domain_left_border; // левая граница ареала обитания (правая граница из макс длины)
  } properties;
  int len; // 0 = head only.
  struct body_pix_t {
    pixel_t pix;
    int pos;
    snake_dir_t dir; //1 or -1
  } body[LEDS_NUMBER];
  struct food_pix_t {
    pixel_t pix;
    int pos;
  } food;
  bool victory_achieved;
} snake_par_t;


int get_new_food_pos(snake_par_t *snake) {
  const int left_border = snake->properties.domain_left_border;
  const int right_border = left_border + snake->properties.max_len;
  bool bad = true;
  int pos = 0;
  uint32_t t0 = GetMs();
  while (bad) {
    bad = false;
    pos = (uint32_t)random(0) % snake->properties.max_len;
    pos += left_border;
    for (int i = 0; i <= snake->len; i++) {
      if (snake->body[i].pos == pos) {
        bad = true;
        break;
      }
    }
    // timeout on rng
    if ((GetMs() - t0) >= _S_TIMEOUT_MS) {
      snake_errors.food_gen++;
      pos = 0;
      break;
    }
  }
  // sanity check
  if ((pos < 0) || (pos > (LEDS_NUMBER - 1))) {
    snake_errors.food_gen++;
    pos = MAX(0, pos);
    pos = MIN(LEDS_NUMBER - 1, pos);
  }
  return pos;
}

void init_snake(snake_par_t *snake, int max_len, int domain_left_border, snake_dir_t initial_dir) {
  // draw smol noodle
  // memset(snake, 0, sizeof(snake_par_t)); // auto clean

  // <MANUAL CLEAN>
  for (int i = 0; i < LEDS_NUMBER; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    b->pos = 0;
    b->dir = FORWARD;
    b->pix.red = 0;
    b->pix.green = 0;
    b->pix.blue = 0;
  }
  snake->food.pos = 100;
  snake->food.pix.red = 100;
  snake->food.pix.green = 100;
  snake->food.pix.blue = 100;
  snake->victory_achieved = false;
  // </MANUAL CLEAN>

  // asserts (not extensive really)
  if (
    !(
      ((initial_dir == BACKWARD) || (initial_dir == FORWARD)) &&
      ((domain_left_border >= 0) && (domain_left_border < (LEDS_NUMBER - 1))) &&
      (max_len <= (LEDS_NUMBER - 1 - domain_left_border)) &&
      (max_len < LEDS_NUMBER)
    )
  ) {
    snake_errors.init_params_error++;
    state.flags.paused = true;
    return;
  }

  snake->properties.domain_left_border = domain_left_border;
  snake->properties.max_len = max_len;
  snake->len = 2;
  for (int i = 0; i <= snake->len; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    b->dir = initial_dir;
    switch (initial_dir) {
    case FORWARD:
      b->pos = domain_left_border + snake->len - i;
      break;
    case BACKWARD:
      b->pos = (domain_left_border + max_len - 1) - snake->len + i;
      break;
    }
    // sanity check
    if ((b->pos >= LEDS_NUMBER) || (b->pos < 0)) {
      snake_errors.init_wrong_position++;
      state.flags.paused = true;
      b->pos = MAX(0, b->pos);
      b->pos = MIN(LEDS_NUMBER - 1, b->pos);
    }
    set_random_pixel_color(&(b->pix));
  }
  set_pix_color(&(snake->body[0].pix), 255, 0, 0); // head is red
  // spawn new food
  snake->food.pos = get_new_food_pos(snake);
  set_random_pixel_color(&(snake->food.pix));
  snake->victory_achieved = false;
}

void snake_baseline(snake_par_t *snake, pixel_t *pix) {
  struct body_pix_t *head = &(snake->body[0]);
  int left_border = snake->properties.domain_left_border;
  int right_border = left_border + snake->properties.max_len;
  // move body
  for (int i = 0; i <= snake->len; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    int new_pos = b->pos + b->dir;
    b->pos = new_pos;
    if ((new_pos == left_border) || (new_pos == right_border)) {
      b->dir *= -1; // flip dir when head hits the zone borders
    }
  }
  // eat food
  if (head->pos == snake->food.pos) {
    // expand snake
    struct body_pix_t *tail = &(snake->body[snake->len]);
    snake->len++;
    struct body_pix_t *new_pix = &(snake->body[snake->len]);
    if ((tail->pos == left_border) || (tail->pos == right_border)) {
      // spawned on the pivot point
      new_pix->pos = tail->pos;
      new_pix->dir = tail->dir * -1;
    } else {
      // normal case
      new_pix->pos = tail->pos - tail->dir;
      new_pix->dir = tail->dir;
    }
    // sanity check
    if ((new_pix->pos < 0) || (new_pix->pos > (LEDS_NUMBER - 1 ))) {
      snake_errors.wrong_position++;
      new_pix->pos = MAX(0, new_pix->pos);
      new_pix->pos = MIN(LEDS_NUMBER - 1, new_pix->pos);
    }
    copy_pix_color(&(new_pix->pix), &(snake->food.pix));
    // spawn new food
    snake->food.pos = get_new_food_pos(snake);
    set_random_pixel_color(&(snake->food.pix));
  }
  // win
  if (snake->len >= (snake->properties.max_len)) {
    // TODO cool animated sequence
    snake->victory_achieved = true;
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
  static snake_par_t snake0;
  if (state.recently_switched_algo || snake0.victory_achieved) {
    init_snake(&snake0, LEDS_NUMBER - 1, 0, FORWARD);
  }
  clear_pixels(pix);
  snake_baseline(&snake0, pix);
}


void two_noodles(pixel_t *pix) {
  static snake_par_t snake1, snake2;
  if (state.recently_switched_algo) {
    init_snake(&snake1, 100, 0, FORWARD);
    init_snake(&snake2, 99, 100, BACKWARD);
  }
  if (snake1.victory_achieved) {
    init_snake(&snake1, 100, 0, FORWARD);
  }
  if (snake2.victory_achieved) {
    init_snake(&snake2, 99, 100, BACKWARD);
  }
  clear_pixels(pix);
  snake_baseline(&snake1, pix);
  snake_baseline(&snake2, pix);
}
