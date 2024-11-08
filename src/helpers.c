#include "helpers.h"

inline void clear_pixels(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = 0;
    pix[i].green = 0;
    pix[i].blue = 0;
  }
}

inline void set_pix_color(pixel_t *pix, uint8_t r, uint8_t g, uint8_t b) {
  pix->red = r;
  pix->green = g;
  pix->blue = b;
}

inline void set_pix_color_arr(pixel_t *pix, const uint8_t *rgb) {
  pix->red = rgb[0];
  pix->green = rgb[1];
  pix->blue = rgb[2];
}

inline void copy_pix_color(pixel_t *pix_dest, pixel_t *pix_source ) {
  pix_dest->red = pix_source->red;
  pix_dest->green = pix_source->green;
  pix_dest->blue = pix_source->blue;
}

void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds) {
  /* Сияние слева и справа от заданных границ (не включительно) */
  // устойчиво к выходам за пределы массива
  for (uint8_t i = 1; i <= num_glowing_leds; i++) {
    uint16_t led_ind;
    uint8_t brightness;
    brightness = 255 * state.brightness / (i * i);
    if (ind_left > i) {
      led_ind = ind_left - i;
      pix[led_ind].red += brightness;
      pix[led_ind].green += brightness;
      pix[led_ind].blue += brightness;
    }
    if (ind_right < (LEDS_NUMBER - i)) {
      led_ind = ind_right + i;
      pix[led_ind].red += brightness;
      pix[led_ind].green += brightness;
      pix[led_ind].blue += brightness;
    }
  }
}

inline float get_delta_period(const uint32_t period) {
  uint32_t delta_t = state.ms - state.last_ms;
  return (float)(delta_t % period) / period;
}

// Linear Congruential Generator (LCG)
// Constants for the LCG (parameters from Numerical Recipes)
#define LCG_A 1664525
#define LCG_C 1013904223
#define LCG_M 0xFFFFFFFF // 2^32
uint32_t random(uint32_t new_seed) {
  static uint32_t seed = 12345;
  if (new_seed != 0)
    seed = new_seed;
  seed = (LCG_A * seed + LCG_C) % LCG_M;
  return (uint8_t)(seed & 0xFF);
}

// uint32_t random(uint32_t new_seed) {
//   return 50;
// }
