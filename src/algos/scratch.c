#include "main.h"
#include "helpers.h"
#include <string.h>
#include "algos.h"

typedef enum {PHASE_UP, PHASE_DOWN} phase_t;

#define _SCR_LEN 10
typedef struct {
  bool initialized;
  uint8_t scratch_countdown;
  uint8_t phase;
  float pos;
  float scratch_speed;
  uint8_t scratch_phase;
  uint8_t colors[_SCR_LEN][3];
} rc_params_t;

uint16_t baseline_scratch(pixel_t *pix) {
  // бегает в две стороны и скрэтчит
  // возвращает индекс начала колонки 
  const float br = state.brightness;
  const uint8_t max_br = 255 * br;
  const double rc_scratch_threshold1 = 0.8;
  const double rc_scratch_threshold2 = 0.6;
  const uint8_t rc_scratch_reset_value = 7;
  static rc_params_t par = {
    .initialized = false,
    .scratch_phase = 4,
  };
  if (par.initialized == false) {
    par.initialized = true;
    uint8_t colors[_SCR_LEN][3] = {
      {60, 122, 108},
      {52, 16, 120},
      {120, 16, 116},
      {128, 16, 66},
      {128, 0, 0},
      {128, 0, 0},
      {128, 16, 66},
      {120, 16, 116},
      {52, 16, 120},
      {60, 122, 108},
    };
    memcpy(par.colors, colors, sizeof(par.colors));
    par.scratch_speed = state.speed;
  }

  if (par.pos > 1) {
    par.phase = PHASE_DOWN;
    par.pos = 1;
    if (par.scratch_countdown > 0)
      par.scratch_countdown--;
  }
  if (par.pos < 0) {
    par.phase = PHASE_UP;
    par.pos = 0;
    if (par.scratch_countdown > 0)
      par.scratch_countdown--;
  }

  if (par.scratch_countdown == 0) { // скрэтч тайм!
    if ((par.phase == PHASE_UP) & (par.pos > rc_scratch_threshold1)) {
      par.phase = PHASE_DOWN;
      if (par.scratch_phase == 0)
        par.scratch_speed *= 2;
      par.scratch_phase++;

    }
    if ((par.phase == PHASE_DOWN) & (par.pos < (rc_scratch_threshold2))) {
      par.phase = PHASE_UP;
      if (par.scratch_phase == 0)
        par.scratch_speed *= 2;
      par.scratch_phase++;
    }
    if (par.scratch_phase >= 4) { // скрэтч офф =(
      par.scratch_countdown = rc_scratch_reset_value;
      par.scratch_phase = 0;
      par.scratch_speed = state.speed;
    }
  } else {
    par.scratch_speed = state.speed; //скорость меняется только не в фазу скретча
  }


  uint32_t period = (uint32_t)((float)2000 / par.scratch_speed);
  if (par.phase == PHASE_UP)
    par.pos += get_delta_period(period);
  else
    par.pos -= get_delta_period(period);

  int ind = (int)(floor(par.pos * LEDS_NUMBER));

  const uint8_t red[3] = {max_br, 0, 0};
  for (uint16_t i = 0; i < _SCR_LEN; i++) {
    int z = (ind + i);
    if (!(z < LEDS_NUMBER)) {
      // Правая граница
      set_pix_color(&pix[LEDS_NUMBER], red);
    } else if (!(z > 0)) {
      //z отрицательное. Левая граница
      set_pix_color(&pix[0], red);
    } else {
      //всё нормально
      set_pix_color(&pix[z], par.colors[i]);
    }
  }
  return ind;
}

void scratch(pixel_t *pix) {
  clear_pixels(pix);
  uint16_t ind = baseline_scratch(pix);
  glowing_sides(pix, ind, ind + _SCR_LEN - 1, 10);
}

void scratch_breathing(pixel_t *pix) {
  breath_colors2(pix);
  baseline_scratch(pix);
}
