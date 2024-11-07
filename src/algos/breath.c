#include "main.h"
#include "helpers.h"

typedef struct {
  bool initialized;
  float pos; // position inside period 0..1
  uint8_t *table;
} bc_params_t;

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

void breath_colors(pixel_t *pix) {
  static bc_params_t par = {
    .initialized = false,
    .pos = 0,
  };
  if (par.initialized == false) {
    par.table = get_sin_wave_table(false);
    par.initialized = true;
  }
  const uint32_t period = (uint32_t)((float)2000 / state.speed);
  par.pos += get_delta_period(period);
  par.pos = fmodf(par.pos, 1); // не более 1
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    uint16_t ri, gi, bi;
    ri = (uint16_t)(i + par.pos * LEDS_NUMBER) % LEDS_NUMBER;
    gi = (ri + LEDS_NUMBER / 3) % LEDS_NUMBER;
    bi = (ri + LEDS_NUMBER * 2 / 3) % LEDS_NUMBER;
    pix[i].red = par.table[ri] + par.table[gi];
    pix[i].green = par.table[gi] + par.table[bi];
    pix[i].blue = par.table[bi] + par.table[ri];
  }
}
