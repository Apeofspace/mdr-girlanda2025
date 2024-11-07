#include "helpers.h"

inline void clear_pixels(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = 0;
    pix[i].green = 0;
    pix[i].blue = 0;
  }
}

inline void set_pix_color(pixel_t *pix, uint8_t *rgb) {
  pix->red = rgb[0];
  pix->green = rgb[1];
  pix->blue = rgb[2];
}

void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds) {
  /* Сияние слева и справа от заданных границ (не включительно) */
  for (uint8_t i = 1; i <= num_glowing_leds; i++) {
    uint16_t led_ind;
    uint8_t brightness;
    brightness = 255 * state.brightness / (i * i);
    if (ind_left > i) {
      led_ind = ind_left - i;
      pix[led_ind].red = brightness;
      pix[led_ind].green = brightness;
      pix[led_ind].blue = brightness;
    }
    if (ind_right < (LEDS_NUMBER - i)) {
      led_ind = ind_right + i;
      pix[led_ind].red = brightness;
      pix[led_ind].green = brightness;
      pix[led_ind].blue = brightness;
    }
  }
}

inline float get_delta_period(const uint32_t period) {
  uint32_t delta_t = state.ms - state.last_ms;
  return (float)(delta_t % period) / period;
}
