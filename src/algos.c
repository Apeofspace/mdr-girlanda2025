#include "algos.h"
#include "delay.h"
#include "main.h"
#include "math.h"

static void clear_pixels(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = 0;
    pix[i].green = 0;
    pix[i].blue = 0;
  }
}

static void set_pix_color(pixel_t *pix, uint8_t *rgb) {
  pix->red = rgb[0];
  pix->green = rgb[1];
  pix->blue = rgb[2];
}

static void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds) {
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

static uint8_t* get_sin_wave_table(bool force_create_table) {
  /* Заполняет таблицу цветами по синусоиде и возвращает указатель на неё */
  static uint8_t sin_wave_table[LEDS_NUMBER];
  static bool table_created = false;
  static float wavelength = 100;
  if (!(table_created) || (force_create_table)) {
    for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
      sin_wave_table[i] = 255 * state.brightness * sin(M_TWOPI * i / wavelength);
    }
    table_created = true;
  }
  return sin_wave_table;
}

inline static float get_delta_period(const uint32_t period) {
  uint32_t delta_t = state.ms - state.last_ms;
  return (float)(delta_t % period) / period;
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
  bct_period += get_delta_period(period);
  bct_period = fmodf(bct_period, 1); // не более 1
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    uint16_t ri, gi, bi;
    ri = (uint16_t)(i + bct_period * LEDS_NUMBER) % LEDS_NUMBER;
    gi = (ri + LEDS_NUMBER / 3) % LEDS_NUMBER;
    bi = (ri + LEDS_NUMBER * 2 / 3) % LEDS_NUMBER;
    pix[i].red = table[ri] + table[gi];
    pix[i].green = table[gi] + table[bi];
    pix[i].blue = table[bi] + table[ri];
  }
}

void lenochka(pixel_t *pix) {
  static uint8_t l_colors1[][3] = {
    {128, 0, 0}, // middle and extensions (extensions go both sides)
    {128, 0, 0},
    {128, 0, 0},
    {128, 0, 0},
    {128, 0, 0},
  };
  static uint8_t l_colors2[][3] = {
    {0, 0, 128}, // middle and extensions (extensions go both sides)
    {0, 0, 128},
    {0, 0, 128},
    {0, 0, 128},
    {0, 0, 128},
  };
  clear_pixels(pix);
  float hop_loc = 0.685;
  static volatile bool hopped = false;
  uint16_t extensions, glow_width = 8;
  uint16_t i1, i2;
  int32_t ex_l1, ex_r1, ex_l2, ex_r2;
  const uint32_t period = (uint32_t)((float)4000 / state.speed);
  static float l_period = 0; // положение внутри периода
  l_period += get_delta_period(period);
  if (l_period > 1) {
    hopped = false;
    l_period = fmodf(l_period, 1.0f); // не более 1
  }

  // middles
  i1 = (uint16_t)(LEDS_NUMBER * l_period) % (LEDS_NUMBER - extensions);
  i2 = (LEDS_NUMBER - i1);
  // extensions
  // extensions = (sizeof(l_colors1) / sizeof(l_colors1[0])) - 1;
  extensions = 4;
  for (uint16_t i = 0; i <= extensions; i++) {
    ex_l1 = ((i1 - i) < 0) ? 0 : i1 - i;
    ex_r1 = ((i1 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i1 + i;
    ex_l2 = ((i2 - i) < 0) ? 0 : i2 - i;
    ex_r2 = ((i2 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i2 + i;
    if (!hopped) {
      pix[ex_l1].red = 255 * state.brightness;
      pix[ex_r1].red = 255 * state.brightness;
      pix[ex_l2].blue = 255 * state.brightness;
      pix[ex_r2].blue = 255 * state.brightness;
      // set_pix_color(&pix[ex_l1], l_colors1[i]);
      // set_pix_color(&pix[ex_r1], l_colors1[i]);
      // set_pix_color(&pix[ex_l2], l_colors2[i]);
      // set_pix_color(&pix[ex_r2], l_colors2[i]);
    } else {
      // set_pix_color(&pix[ex_l1], l_colors2[i]);
      // set_pix_color(&pix[ex_r1], l_colors2[i]);
      // set_pix_color(&pix[ex_l2], l_colors1[i]);
      // set_pix_color(&pix[ex_r2], l_colors1[i]);
      pix[ex_l1].blue = 255 * state.brightness;
      pix[ex_r1].blue = 255 * state.brightness;
      pix[ex_l2].red = 255 * state.brightness;
      pix[ex_r2].red = 255 * state.brightness;
    }
  }
  glowing_sides(pix, ex_l1, ex_r1, glow_width);
  glowing_sides(pix, ex_l2, ex_r2, glow_width);

  if ((l_period >= hop_loc) && !(hopped)) {
    // custom sequence
    state.flags.paused = true;
    hopped = true;
    send_pixels();
    MDR_Delay(300);
    uint16_t orig_ext = extensions;
    uint16_t orig_glow_w = glow_width;
    // reduce
    while (extensions) {
      extensions--;
      clear_pixels(pix);
      for (uint16_t i = 0; i < extensions; i++) {
        ex_l1 = ((i1 - i) < 0) ? 0 : i1 - i;
        ex_r1 = ((i1 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i1 + i;
        ex_l2 = ((i2 - i) < 0) ? 0 : i2 - i;
        ex_r2 = ((i2 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i2 + i;
        // set_pix_color(&pix[ex_l1], l_colors1[i]);
        // set_pix_color(&pix[ex_r1], l_colors1[i]);
        // set_pix_color(&pix[ex_l2], l_colors2[i]);
        // set_pix_color(&pix[ex_r2], l_colors2[i]);
        pix[ex_l1].red = 255 * state.brightness;
        pix[ex_r1].red = 255 * state.brightness;
        pix[ex_l2].blue = 255 * state.brightness;
        pix[ex_r2].blue = 255 * state.brightness;
      }
      if (extensions) {
        glow_width -= 2;
        glowing_sides(pix, ex_l1, ex_r1, glow_width);
        glowing_sides(pix, ex_l2, ex_r2, glow_width);
      }
      send_pixels();
      MDR_Delay(400);
    }
    MDR_Delay(600);
    // enlarge
    while (extensions < orig_ext) {
      clear_pixels(pix);
      for (uint16_t i = 0; i <= extensions; i++) {
        ex_l1 = ((i1 - i) < 0) ? 0 : i1 - i;
        ex_r1 = ((i1 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i1 + i;
        ex_l2 = ((i2 - i) < 0) ? 0 : i2 - i;
        ex_r2 = ((i2 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i2 + i;
        // set_pix_color(&pix[ex_l1], l_colors2[i]);
        // set_pix_color(&pix[ex_r1], l_colors2[i]);
        // set_pix_color(&pix[ex_l2], l_colors1[i]);
        // set_pix_color(&pix[ex_r2], l_colors1[i]);
        pix[ex_l1].blue = 255 * state.brightness;
        pix[ex_r1].blue = 255 * state.brightness;
        pix[ex_l2].red = 255 * state.brightness;
        pix[ex_r2].red = 255 * state.brightness;
      }
      extensions++;
      glow_width += 2;
      glowing_sides(pix, ex_l1, ex_r1, glow_width);
      glowing_sides(pix, ex_l2, ex_r2, glow_width);
      send_pixels();
      MDR_Delay(300);
    }
    state.last_ms = GetMs();
    state.ms = GetMs();
    state.flags.paused = false;
  }

}

