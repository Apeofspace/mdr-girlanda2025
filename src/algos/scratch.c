#include "main.h"
#include "helpers.h"

typedef struct {
  uint16_t prev_index;
  uint8_t (*colors)[3];
} bc_params_t;

void scratch(pixel_t* pix) {
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

void* init_scratch() {
  static uint8_t colors[][3] = {
    {168, 50, 109},
    {182, 33, 107},
    {182, 33, 107},
    {168, 50, 109},
  };
  static bc_params_t params;
  params.colors = colors;
  return scratch;
}

