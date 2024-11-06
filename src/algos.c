#include "algos.h"
#include "main.h"
#include "math.h"

static void clear_pixels(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = 0;
    pix[i].green = 0;
    pix[i].blue = 0;
  }
}

static void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds) {
  /* Сияние слева и справа от заданных границ (не включительно) */
  for (uint8_t i = 1; i <= num_glowing_leds; i++) {
    uint16_t led_ind;
    uint8_t brightness;
    led_ind = (ind_left <= i) ? LEDS_NUMBER - i : ind_left - i;
    brightness = MAX_BRIGHTNESS / (i * i);
    pix[led_ind].red = brightness;
    pix[led_ind].green = brightness;
    pix[led_ind].blue = brightness;
    led_ind = (ind_right >= (LEDS_NUMBER - i)) ? i : ind_right + i;
    pix[led_ind].red = brightness;
    pix[led_ind].green = brightness;
    pix[led_ind].blue = brightness;
  }
}

void all_red(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = MAX_BRIGHTNESS;
    pix[i].green = 0;
    pix[i].blue = 0;
  }
}

void all_white(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = MAX_BRIGHTNESS;
    pix[i].green = MAX_BRIGHTNESS;
    pix[i].blue = MAX_BRIGHTNESS;
  }
}

void running_red_dot(pixel_t *pix) {
  /* Просто красная точка бегает */
  clear_pixels(pix); // очистить
  // допустим весь период две секунды при скорости 128
  float period = 1000 * (float)255 / state.speed;
  float k = (state.ms % (uint32_t)period) / period;
  k = (k > 1) ? 1 : k;
  k = (k < 0) ? 0 : k;
  uint16_t led_index_m = (uint16_t)(LEDS_NUMBER * k);
  uint16_t led_index_r = (led_index_m == LEDS_NUMBER) ? 0 : led_index_m + 1;
  uint16_t led_index_l = (led_index_m == 0) ? LEDS_NUMBER : led_index_m - 1;
  pix[led_index_m].red = MAX_BRIGHTNESS;
  pix[led_index_m].green = 0;
  pix[led_index_m].blue = 0;
  pix[led_index_r].red = MAX_BRIGHTNESS;
  pix[led_index_r].green = 0;
  pix[led_index_r].blue = 0;
  pix[led_index_l].red = MAX_BRIGHTNESS;
  pix[led_index_l].green = 0;
  pix[led_index_l].blue = 0;
  glowing_sides(pix, led_index_l, led_index_r, 3);
}

