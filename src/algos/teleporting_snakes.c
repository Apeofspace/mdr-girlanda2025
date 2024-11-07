#include "main.h"
#include "helpers.h"

#define _L_WIDTH 5
#define _L_GLOW_WIDTH 8
typedef struct {
  float pos;
  float hop_pos;
  bool hopped;
  uint8_t colors1[_L_WIDTH][3];
  uint8_t colors2[_L_WIDTH][3];
} tps_params_t;

void teleporting_snakes(pixel_t *pix) {
  static tps_params_t par = {
    .hopped = false,
    .pos = 0,
    .hop_pos = 0.685,
    .colors1 = {
      {128, 0, 0}, // middle and extensions (extensions go both sides)
      {128, 0, 0},
      {128, 0, 0},
      {128, 0, 0},
      {128, 0, 0},
    },
    .colors2 = {
      {0, 0, 128}, // middle and extensions (extensions go both sides)
      {0, 0, 128},
      {0, 0, 128},
      {0, 0, 128},
      {0, 0, 128},
    },
  };
  clear_pixels(pix);
  uint16_t extensions, glow_width = _L_GLOW_WIDTH;
  uint16_t i1, i2;
  int32_t ex_l1, ex_r1, ex_l2, ex_r2;
  const uint32_t period = (uint32_t)((float)4000 / state.speed);
  par.pos += get_delta_period(period);
  if (par.pos > 1) {
    par.hopped = false;
    par.pos = fmodf(par.pos, 1.0f); // не более 1
  }

  // middles
  extensions = _L_WIDTH - 1;
  i1 = (uint16_t)(LEDS_NUMBER * par.pos) % (LEDS_NUMBER - extensions);
  i2 = (LEDS_NUMBER - i1);
  // extensions
  for (uint16_t i = 0; i <= extensions; i++) {
    ex_l1 = ((i1 - i) < 0) ? 0 : i1 - i;
    ex_r1 = ((i1 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i1 + i;
    ex_l2 = ((i2 - i) < 0) ? 0 : i2 - i;
    ex_r2 = ((i2 + i) > LEDS_NUMBER) ? LEDS_NUMBER : i2 + i;
    if (!par.hopped) {
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

  if ((par.pos >= par.hop_pos) && !(par.hopped)) {
    // custom sequence
    state.flags.paused = true;
    par.hopped = true;
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
