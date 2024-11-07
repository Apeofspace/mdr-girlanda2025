#include "main.h"
#include "helpers.h"

void running_red_dot(pixel_t *pix) {
  /* Просто красная точка бегает */
  clear_pixels(pix); // очистить
  // допустим весь период две секунды при скорости 128
  float period = 3000 / state.speed;
  float k = (state.ms % (uint32_t)period) / period;
  k = (k > 1) ? 1 : k;
  k = (k < 0) ? 0 : k;
  uint16_t led_index_m = (uint16_t)(LEDS_NUMBER * k);
  uint16_t led_index_r = (led_index_m == LEDS_NUMBER) ? 0 : led_index_m + 1;
  uint16_t led_index_l = (led_index_m == 0) ? LEDS_NUMBER : led_index_m - 1;
  pix[led_index_m].red = 255 * state.brightness;
  pix[led_index_m].green = 0;
  pix[led_index_m].blue = 0;
  pix[led_index_r].red = 255 * state.brightness;
  pix[led_index_r].green = 0;
  pix[led_index_r].blue = 0;
  pix[led_index_l].red = 255 * state.brightness;
  pix[led_index_l].green = 0;
  pix[led_index_l].blue = 0;
  glowing_sides(pix, led_index_l, led_index_r, 4);
}

