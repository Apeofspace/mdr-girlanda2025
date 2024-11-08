#include "delay.h"
#include "main.h"
#include "helpers.h"

// NOTE pixels can be same pos or same dir, but never both
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

void set_random_pixel_color(pixel_t *pix) {
  // WARN may not be very random
  pix->red = random(0) % (uint16_t)(255 * state.brightness);
  pix->green = random(0) % (uint16_t)(255 * state.brightness);
  pix->blue = random(0) % (uint16_t)(255 * state.brightness);
}

int get_new_food_pos(snake_par_t *snake) {
  bool bad = true;
  int pos = 0;
  while (bad) {
    bad = false;
    pos = (uint32_t)random(0) % LEDS_NUMBER;
    for (int i = 0; i <= snake->len; i++) {
      if (snake->body[i].pos == pos) {
        bad = true;
        break;
      }
    }
  }
  // sanity check
  pos = MAX(0, pos);
  pos = MIN(LEDS_NUMBER, pos);
  return pos;
}

void init_snake(snake_par_t *snake) {
  // draw smol noodle
  snake->len = 2;
  for (int i = 0; i <= snake->len; i++) {
    struct body_pix_t *b = &(snake->body[i]);
    b->pos = 4 - i;
    b->dir = 1;
    set_random_pixel_color(&(b->pix));
  }
  set_pix_color(&(snake->body[0].pix), 255, 0, 0); // head is red
  // spawn new food
  random(GetMs());
  snake->food.pos = get_new_food_pos(snake);
  set_random_pixel_color(&(snake->food.pix));
}

void snake_baseline(pixel_t *pix) {
  static snake_par_t snake;
  struct body_pix_t *head = &(snake.body[0]);
  // init
  if (state.recently_switched_algo) {
    init_snake(&snake);
  }
  // move body
  for (int i = 0; i <= snake.len; i++) {
    struct body_pix_t *b = &(snake.body[i]);
    int new_pos = b->pos + b->dir;
    b->pos = new_pos;
    if ((new_pos == 0) || (new_pos == LEDS_NUMBER)) {
      b->dir *= -1; // flip dir when hits the ends
    }
  }
  // eat food
  if (head->pos == snake.food.pos) {
    // expand snake
    struct body_pix_t *tail = &(snake.body[snake.len]);
    snake.len++;
    struct body_pix_t *new_pix = &(snake.body[snake.len]);
    if ((tail->pos == 0) || (tail->pos == LEDS_NUMBER)) {
      // spawned on the pivot point
      new_pix->pos = tail->pos;
      new_pix->dir = tail->dir * -1;
    } else {
      // normal case
      new_pix->pos = tail->pos - tail->dir;
      new_pix->dir = tail->dir;
    }
    copy_pix_color(&(new_pix->pix), &(snake.food.pix));
    // spawn new food
    snake.food.pos = get_new_food_pos(&snake);
    set_random_pixel_color(&(snake.food.pix));
  }
  // win
  if (snake.len >= LEDS_NUMBER - 2) {
    // placeholder
    state.recently_switched_algo = true; // force reinit
  }
  // draw everything
  for (int i = snake.len; i >= 0; i--) {
    struct body_pix_t *b = &(snake.body[i]);
    copy_pix_color(&(pix[b->pos]), &(b->pix));
  }
  // draw food
  copy_pix_color(&(pix[snake.food.pos]), &(snake.food.pix));
  // glowing_sides(pix, snake.food.pos, snake.food.pos, 2);
}

void danger_noodle(pixel_t *pix) {
  clear_pixels(pix);
  snake_baseline(pix);
}
