#include "main.h"
#include "helpers.h"

#define _S_MS_PER_STEP 10


struct sn_errors_t {
  volatile uint32_t food_gen_pos;
  volatile uint32_t init_params_error;
  volatile uint32_t init_wrong_position;
  volatile uint32_t wrong_position;
  volatile uint32_t victory_achieved ;
} snake_errors = {
  .food_gen_pos = 0,
  .init_params_error = 0,
  .victory_achieved = 0,
};

typedef enum {FORWARD = 1, BACKWARD = -1} snake_dir_t;

typedef struct {
  pixel_t color;
  int pos;
  bool eaten;
} snake_food_t;

typedef struct {
  int body_len; // 0 = head only.
  bool max_size_achieved;
  snake_food_t *food;
  struct sn_borders_t {
    int left;
    int right;
  } borders;
  struct sn_body_segment_t {
    pixel_t color;
    int pos;
    snake_dir_t dir; // 1 or -1
  } body[LEDS_NUMBER];
} snake_par_t;

float _s_steps = 0;
snake_food_t snake_food;
snake_par_t snake1, snake2;
snake_par_t *all_snakes[2] = {&snake1, &snake2};

int get_new_food_pos(snake_par_t *snakes[], uint8_t snake_count) {
  int possible_foods[snake_count];
  // make a list of possible food positions for each snake
  for (int i = 0; i < snake_count; i++) {
    snake_par_t *snake = snakes[i];
    const int left_border = snake->borders.left;
    const int right_border = snake->borders.right;
    const int max_len = right_border - left_border;
    struct sn_body_segment_t *head = &snake->body[0];
    struct sn_body_segment_t *tail = &snake->body[snake->body_len];
    // get body coordinates
    int body_leftmost = MIN(head->pos, tail->pos);
    int body_rightmost = MAX(head->pos, tail->pos);
    bool pivoting = (head->dir != tail->dir);
    if (pivoting) {
      body_leftmost = (head->dir == FORWARD) ? left_border : body_leftmost;
      body_rightmost = (head->dir == FORWARD) ? body_rightmost : right_border;
    }
    // get free spaces count
    int free_space = (body_leftmost - left_border) + (right_border - body_rightmost);
    int taken_space = max_len - free_space + 1; // + 1 for head
    // generate food position
    int food_pos = (uint32_t)random(0) % free_space;
    food_pos += left_border;
    if (food_pos >= body_leftmost) {
      food_pos += taken_space;
    }
    // sanity check
    if ((food_pos < left_border) || (food_pos > right_border)) {
      snake_errors.food_gen_pos++;
      food_pos = snake->borders.left;
    }
    possible_foods[i] = food_pos;
  }
  // choose one random food position
  return possible_foods[(uint32_t)random(0) % snake_count];
}

void spawn_new_food(snake_food_t *food, snake_par_t *snakes[], uint8_t snake_count) {
  food->pos = get_new_food_pos(snakes, snake_count);
  set_random_pixel_color(&(food->color));
  food->eaten = false;
}

void init_snake(snake_par_t *snake, snake_food_t *food, int left_border_pos, int right_border_pos,
                snake_dir_t initial_dir) {
  // draw smol noodle
  // memset(snake, 0, sizeof(snake_par_t)); // auto clean

  // <MANUAL CLEAN>
  for (int i = 0; i < LEDS_NUMBER; i++) {
    struct sn_body_segment_t *b = &(snake->body[i]);
    b->pos = 0;
    b->dir = FORWARD;
    b->color.red = 0;
    b->color.green = 0;
    b->color.blue = 0;
  }
  snake->max_size_achieved = false;
  // </MANUAL CLEAN>

  // asserts (not extensive really)
  if (
    !(
      ((initial_dir == BACKWARD) || (initial_dir == FORWARD)) &&
      (left_border_pos >= 0) && (left_border_pos < right_border_pos) &&
      (right_border_pos <= (LEDS_NUMBER - 1))
    )
  ) {
    snake_errors.init_params_error++;
    state.flags.paused = true;
    return;
  }

  snake->food = food;
  snake->borders.left = left_border_pos;
  snake->borders.right = right_border_pos;
  snake->body_len = 2;
  for (int i = 0; i <= snake->body_len; i++) {
    struct sn_body_segment_t *b = &(snake->body[i]);
    b->dir = initial_dir;
    switch (initial_dir) {
    case FORWARD:
      b->pos = left_border_pos + snake->body_len - i;
      break;
    case BACKWARD:
      b->pos = right_border_pos - snake->body_len + i;
      break;
    }
    // sanity check
    if ((b->pos >= LEDS_NUMBER) || (b->pos < 0)) {
      snake_errors.init_wrong_position++;
      state.flags.paused = true;
      b->pos = MAX(0, b->pos);
      b->pos = MIN(LEDS_NUMBER - 1, b->pos);
    }
    set_random_pixel_color(&(b->color));
  }
  set_pix_color(&(snake->body[0].color), 255, 0, 0); // head is red
}

void snake_step(snake_par_t *snake, pixel_t *pix) {
  struct sn_body_segment_t *head = &(snake->body[0]);
  int left_border = snake->borders.left;
  int right_border = snake->borders.right;
  int max_len = right_border - left_border;
  // MOVE BODY
  for (int i = 0; i <= snake->body_len; i++) {
    struct sn_body_segment_t *b = &(snake->body[i]);
    int new_pos = b->pos + b->dir;
    b->pos = new_pos;
    if ((new_pos == left_border) || (new_pos == right_border)) {
      b->dir *= -1; // flip dir when head hits the zone borders
    }
  }
  // EAT FOOD
  if (head->pos == snake->food->pos) {
    snake->food->eaten = true;
    // expand snake
    struct sn_body_segment_t *tail = &(snake->body[snake->body_len]);
    snake->body_len++;
    struct sn_body_segment_t *new_pix = &(snake->body[snake->body_len]);
    if ((tail->pos == left_border) || (tail->pos == right_border)) {
      // spawned on the pivot point
      // NOTE pivot points are special.
      // two pixels can't share the same position there, so we go back 1
      new_pix->pos = tail->pos + tail->dir; // go backwards 1
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
    copy_pix_color(&(new_pix->color), &(snake->food->color));
    // win?
    if (snake->body_len >= max_len) {
      // TODO cool animated sequence
      snake->max_size_achieved = true;
      snake_errors.victory_achieved++;
    }
  }
  // DRAW BODY
  for (int i = snake->body_len; i >= 0; i--) {
    struct sn_body_segment_t *b = &(snake->body[i]);
    copy_pix_color(&(pix[b->pos]), &(b->color));
  }
  // DRAW FOOD
  copy_pix_color(&(pix[snake->food->pos]), &(snake->food->color));
  // glowing_gauss(pix, snake->food->pos, snake->food->pos, 7, 2);
}

void danger_noodle(pixel_t *pix) {
  if (state.recently_switched_algo || snake1.max_size_achieved) {
    init_snake(&snake1, &snake_food, 0, LEDS_NUMBER - 1, FORWARD);
    spawn_new_food(&snake_food, all_snakes, 1);
    _s_steps = 0;
  }
  _s_steps += get_delta_steps(_S_MS_PER_STEP);
  while (_s_steps >= 1) {
    _s_steps--;
    if (snake_food.eaten) {
      spawn_new_food(&snake_food, all_snakes, 1);
    }
    clear_pixels(pix);
    snake_step(&snake1, pix);
  }
}

void two_noodles(pixel_t *pix) {
  if (state.recently_switched_algo) {
    init_snake(&snake1, &snake_food, 0, 99, FORWARD);
    init_snake(&snake2, &snake_food, 100, 199, FORWARD);
    snake_food.eaten = true;
    _s_steps = 0;
  }
  _s_steps += get_delta_steps(_S_MS_PER_STEP);
  while (_s_steps >= 1) {
    _s_steps--;
    if (snake1.max_size_achieved) {
      init_snake(&snake1, &snake_food, 0, 99, FORWARD);
    }
    if (snake2.max_size_achieved) {
      init_snake(&snake2, &snake_food, 100, 199, BACKWARD);
    }
    if (snake_food.eaten) {
      spawn_new_food(&snake_food, all_snakes, 2);
    }
    clear_pixels(pix);
    snake_step(&snake1, pix);
    snake_step(&snake2, pix);
  }
}
