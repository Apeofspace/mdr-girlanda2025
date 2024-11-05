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

