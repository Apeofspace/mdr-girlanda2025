#ifndef __HELPERS_H
#define __HELPERS_H

#include "main.h"

extern volatile uint32_t msSinceStart;
extern bool SysTickInitialized;

typedef enum {
  NOKEY = 0,
  SEL = 1,
  RIGHT = 2,
  LEFT = 3,
  UP = 4,
  DOWN = 5,
  MULTIPLE = 6,
  NUM_KEY_CODES
} KeyCode;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Кнопки */
void init_joystick(void);
KeyCode joystick_get_key(void);

/* Таймер */
#define GetMs() (msSinceStart)
void SysTick_Handler(void);
void init_SysTick();
uint8_t MDR_Delay(uint32_t ms);
float get_delta_period(const uint32_t period);

/* Пиксели */
void clear_pixels(pixel_t *pix);
void set_pix_color_arr(pixel_t *pix, const uint8_t *rgb);
void set_pix_color(pixel_t *pix, uint8_t r, uint8_t g, uint8_t b);
void copy_pix_color(pixel_t *pix_dest, pixel_t *pix_source );
void set_random_pixel_color(pixel_t *pix);
void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds);
void glowing_gauss(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t max_leds, float dispersion);

/* RNG */
uint32_t random(uint32_t new_seed);
void init_RNG();

#endif
