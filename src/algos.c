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
    brightness = 255 * state.brightness / (i * i);
    pix[led_ind].red = brightness;
    pix[led_ind].green = brightness;
    pix[led_ind].blue = brightness;
    led_ind = (ind_right >= (LEDS_NUMBER - i)) ? i : ind_right + i;
    pix[led_ind].red = brightness;
    pix[led_ind].green = brightness;
    pix[led_ind].blue = brightness;
  }
}

static uint8_t* get_sin_wave_table(bool force_create_table) {
  /* Заполняет таблицу цветами по синусоиде и возвращает указатель на неё */
  static uint8_t sin_wave_table[LEDS_NUMBER];
  static bool table_created = false;
  static float wavelength = 50;
  if (!(table_created) || (force_create_table)) {
    for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
      sin_wave_table[i] = 255 * state.brightness * sin(M_TWOPI * i / wavelength);
    }
    table_created = true;
  }
  return sin_wave_table;
}


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

void breath_colors_rgb_table(pixel_t *pix) {
  uint8_t *table = get_sin_wave_table(false);
  const uint32_t period = (uint32_t)((float)2000 / state.speed);
  static float bct_period = 0; // положение внутри периода
  uint32_t delta_t = state.ms - state.last_ms;
  bct_period += (float)(delta_t % period) / period;
  bct_period = fmodf(bct_period, 1); // не более 1
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    uint16_t ri, gi, bi;
    ri = (uint16_t)(i + bct_period * LEDS_NUMBER) % LEDS_NUMBER;
    gi = (ri + LEDS_NUMBER / 3) % LEDS_NUMBER;
    bi = (ri + LEDS_NUMBER * 2 / 3) % LEDS_NUMBER;
    pix[i].red = table[ri];
    pix[i].green = table[gi];
    pix[i].blue = table[bi];
  }
}

