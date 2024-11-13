#include "helpers.h"
#include "MDR32F9Qx_adc.h"

inline void clear_pixels(pixel_t *pix) {
  for (uint16_t i = 0; i < LEDS_NUMBER; i++) {
    pix[i].red = 0;
    pix[i].green = 0;
    pix[i].blue = 0;
  }
}

inline void set_pix_color(pixel_t *pix, uint8_t r, uint8_t g, uint8_t b) {
  pix->red = r;
  pix->green = g;
  pix->blue = b;
}

inline void set_pix_color_arr(pixel_t *pix, const uint8_t *rgb) {
  pix->red = rgb[0];
  pix->green = rgb[1];
  pix->blue = rgb[2];
}

inline void copy_pix_color(pixel_t *pix_dest, pixel_t *pix_source ) {
  pix_dest->red = pix_source->red;
  pix_dest->green = pix_source->green;
  pix_dest->blue = pix_source->blue;
}

void glowing_sides(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t num_glowing_leds) {
  /* Сияние слева и справа от заданных границ (не включительно) */
  // устойчиво к выходам за пределы массива
  for (uint8_t i = 1; i <= num_glowing_leds; i++) {
    uint16_t led_ind;
    uint8_t brightness;
    brightness = 255 * state.brightness / (i * i);
    if (ind_left > i) {
      led_ind = ind_left - i;
      pix[led_ind].red += brightness;
      pix[led_ind].green += brightness;
      pix[led_ind].blue += brightness;
    }
    if (ind_right < (LEDS_NUMBER - i)) {
      led_ind = ind_right + i;
      pix[led_ind].red += brightness;
      pix[led_ind].green += brightness;
      pix[led_ind].blue += brightness;
    }
  }
}

inline float get_delta_period(const uint32_t period) {
  uint32_t delta_t = state.ms - state.last_ms;
  return (float)(delta_t % period) / period;
}

// Linear Congruential Generator (LCG)
// Constants for the LCG (parameters from Numerical Recipes)
#define LCG_A 1664525
#define LCG_C 1013904223
#define LCG_M 0xFFFFFFFF // 2^32
uint32_t _h_seed = 12345;
uint32_t random(uint32_t new_seed) {
  if (new_seed != 0)
    _h_seed = new_seed;
  _h_seed = (LCG_A * _h_seed + LCG_C) % LCG_M;
  return (uint8_t)(_h_seed & 0xFF);
}

int get_noise_from_ADC() {
  // ШУМ для сидирования псевдорандома
  uint32_t t0 = GetMs();
  ADC1_SetChannel(ADC_CH_ADC1);
  ADC1_Start();
  while (!(ADC1_GetStatus() & ADCx_FLAG_END_OF_CONVERSION)) {
    if (GetMs() - t0 > 2) {
      // timeout error
      return 12345;
    }
  }
  return (uint16_t)ADC1_GetResult();
}

void init_ADC_noise() {
  // ADC для генерации рандомного шума
  ADC_InitTypeDef ADC_InitStruct;
  ADCx_InitTypeDef ADCx_InitStruct;
  PORT_InitTypeDef GPIO_InitStruct;
  // /* тактирование на ADC порте D */
  RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTD, ENABLE);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_ADC, ENABLE);

  /* GPIO ADC channels*/
  GPIO_InitStruct.PORT_Pin = PORT_Pin_7;
  GPIO_InitStruct.PORT_OE = PORT_OE_IN;
  GPIO_InitStruct.PORT_FUNC = PORT_FUNC_PORT;
  GPIO_InitStruct.PORT_MODE = PORT_MODE_ANALOG;
  GPIO_InitStruct.PORT_SPEED = PORT_SPEED_MAXFAST;
  PORT_Init(MDR_PORTD, &GPIO_InitStruct);

  /* Переинициализация ADC и структур для настройки ADC */
  ADC_DeInit();
  ADC_StructInit(&ADC_InitStruct);
  ADCx_StructInit(&ADCx_InitStruct);

  ADCx_InitStruct.ADC_DelayGo = 0x7; // доп. задержка, если channel switching
  ADCx_InitStruct.ADC_SamplingMode = ADC_SAMPLING_MODE_SINGLE_CONV;
  ADCx_InitStruct.ADC_ChannelNumber = ADC_CH_ADC7;
  ADCx_InitStruct.ADC_Prescaler = ADC_CLK_div_16; // выбор делителя тактовой частоты

  ADC_Init(&ADC_InitStruct);
  ADC1_Init(&ADCx_InitStruct);
  ADC1_Cmd(ENABLE); // ВКЛЮЧИТЬ АЦП
}

void init_RNG() {
  init_ADC_noise();
  _h_seed = get_noise_from_ADC();
}
