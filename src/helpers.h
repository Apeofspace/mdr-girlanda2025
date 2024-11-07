#ifndef __HELPERS_H
#define __HELPERS_H 

#include "main.h"
#include "delay.h"

void clear_pixels(pixel_t *pix);
void set_pix_color(pixel_t *pix, uint8_t *rgb);
void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds);
float get_delta_period(const uint32_t period);

#endif
