#include "main.h"
#include "helpers.h"
#include "string.h"

typedef struct {
  float pos; // position inside period 0..1
  uint8_t *table;
} bc_params_t;

typedef struct {
  float pos; // position inside period 0..1
  uint8_t table_r[LEDS_NUMBER];
  uint8_t table_g[LEDS_NUMBER];
  uint8_t table_b[LEDS_NUMBER];
} bc2_params_t;

static uint8_t* get_sin_wave_table(bool force_create_table, uint16_t wavelength) {
  /* Заполняет таблицу цветами по синусоиде и возвращает указатель на неё */
  static uint8_t sin_wave_table[LEDS_NUMBER];
  static bool table_created = false; // не пересоздавать таблицу, если уже есть
  if (!(table_created) || (force_create_table)) {
    for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
      sin_wave_table[i] = 255 * state.brightness * sin(M_TWOPI * i / wavelength);
    }
    table_created = true;
  }
  return sin_wave_table;
}

void breath_colors_baseline(pixel_t *pix, uint16_t wavelength) {
  static bc_params_t par_bc = {
    .pos = 0,
  };
  if (state.recently_switched_algo) {
    // переинициализировать таблицу при смене алгоритма
    par_bc.table = get_sin_wave_table(true, wavelength);
  }
  const uint32_t period = (uint32_t)((float)2000 / state.speed);
  par_bc.pos += get_delta_period(period);
  par_bc.pos = fmodf(par_bc.pos, 1); // не более 1
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    uint16_t ri, gi, bi;
    ri = (uint16_t)(i + par_bc.pos * LEDS_NUMBER) % LEDS_NUMBER;
    gi = (ri + LEDS_NUMBER / 3) % LEDS_NUMBER;
    bi = (ri + LEDS_NUMBER * 2 / 3) % LEDS_NUMBER;
    pix[i].red = par_bc.table[ri] + par_bc.table[gi];
    pix[i].green = par_bc.table[gi] + par_bc.table[bi];
    pix[i].blue = par_bc.table[bi] + par_bc.table[ri];
  }
}

void breath_colors2_baseline(pixel_t *pix, uint16_t wavelength, uint16_t base_period) {
  static bc2_params_t par_bc2;
  if (state.recently_switched_algo) {
    // переинициализировать таблицу при смене алгоритма
    memcpy(par_bc2.table_r, get_sin_wave_table(true, wavelength), LEDS_NUMBER);
    memcpy(par_bc2.table_g, get_sin_wave_table(false, wavelength), LEDS_NUMBER);
    memcpy(par_bc2.table_b, get_sin_wave_table(false, wavelength), LEDS_NUMBER);
    par_bc2.pos = 0;
    for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
      // модификаторы
      par_bc2.table_r[i] = par_bc2.table_r[i] * 0.5 + state.brightness * 255;
      par_bc2.table_g[i] = par_bc2.table_r[i] + state.brightness * 255;
      par_bc2.table_b[i] = par_bc2.table_r[i] * 0.33 + state.brightness * 255;
      // ограничение по максимальной яркости
      MAX(par_bc2.table_r[i], (255 * state.brightness));
      MAX(par_bc2.table_g[i], (255 * state.brightness));
      MAX(par_bc2.table_b[i], (255 * state.brightness));
    }
  }
  const uint32_t period = (uint32_t)((float)base_period / state.speed);
  par_bc2.pos += get_delta_period(period);
  par_bc2.pos = fmodf(par_bc2.pos, 1); // не более 1
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    uint16_t ri, gi, bi;
    ri = (uint16_t)(i + par_bc2.pos * LEDS_NUMBER) % LEDS_NUMBER;
    gi = (ri + LEDS_NUMBER / 3) % LEDS_NUMBER;
    bi = (ri + LEDS_NUMBER * 2 / 3) % LEDS_NUMBER;
    pix[i].red = par_bc2.table_r[ri];
    pix[i].green = par_bc2.table_g[gi];
    pix[i].blue = par_bc2.table_b[bi];
  }
}

void breath_colors(pixel_t *pix) {
  breath_colors_baseline(pix, LEDS_NUMBER / 2);
}

void breath_colors2(pixel_t *pix) {
  breath_colors2_baseline(pix, LEDS_NUMBER, 4000);
}
