#include "helpers.h"
#include "MDR32F9Qx_adc.h"
#include "MDR32F9Qx_port.h"
#include <math.h>

volatile uint32_t msSinceStart = 0;
bool SysTickInitialized = false;

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

void set_random_pixel_color(pixel_t *pix) {
  // weighted for extra fun
  uint8_t base_brightness = state.brightness * 255;
  float rweight = 1, bweight = 1, gweight = 1;
  uint8_t weighted_color = random(0) % 3;
  switch (weighted_color) {
  case 0:
    rweight = 0;
    break;
  case 1:
    gweight = 0;
    break;
  case 2:
    bweight = 0;
    break;
  }
  pix->red = random(0) * state.brightness * rweight;
  pix->green = random(0) * state.brightness * gweight;
  pix->blue = random(0) * state.brightness * bweight;
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

void glowing_gauss(pixel_t *pix, uint16_t ind_left, uint16_t ind_right, uint16_t max_leds, float dispersion) {
  /*
  Сияние с нормальным распределением.
  Рекоммендуемые значения max_leds = 5..10, dispersion = 1..3
  */
  const uint8_t max_br = 255 * state.brightness;
  for (uint8_t i = 1; i <= max_leds; i++) {
    float k = 0.39894 / dispersion * expf((-(i ^ 2)) / (2 * dispersion));
    uint8_t brightness = max_br * k;
    uint16_t led_ind;
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
  if (state.last_ms > state.ms)
    return 0;
  return (float)((state.ms - state.last_ms) % period) / period;
}

inline float get_delta_steps(const float ms_per_step) {
  if (state.last_ms > state.ms)
    return 0;
  return ((float)(state.ms - state.last_ms) / ms_per_step) * state.speed;
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

void init_SysTick() {
  msSinceStart = 0;
  SysTick->LOAD = 80000; // 1мс при CPU_clock = 80 MHz
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
  SysTickInitialized = true;
}

/* Count time */
void SysTick_Handler(void) {
  msSinceStart++; // used in all kind of things
}

/* Blocking delay using systick on be92 */
uint8_t MDR_Delay(uint32_t ms) {
  uint8_t res = 0;
  volatile uint32_t t0 = GetMs();
  uint32_t t1 = t0 + ms;
  const uint32_t absolute_timeout = 400000000;
  volatile uint32_t cpu_tick_count = 0;
  if (!(SysTickInitialized)) {
    // TODO error
  } else {
    while (GetMs() < t1) {
      cpu_tick_count++;
      if (cpu_tick_count++ > absolute_timeout) {
        // TODO error
        return 0;
      }
    }
    res = 1;
  }
  return res;
}

void init_joystick(void) {
  PORT_InitTypeDef GPIO_user_init;

  RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTC, ENABLE);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTE, ENABLE);

  GPIO_user_init.PORT_OE = PORT_OE_IN;
  GPIO_user_init.PORT_PULL_DOWN = PORT_PULL_DOWN_ON;
  GPIO_user_init.PORT_SPEED = PORT_SPEED_FAST;
  GPIO_user_init.PORT_FUNC = PORT_FUNC_PORT;
  GPIO_user_init.PORT_MODE = PORT_MODE_DIGITAL;

  GPIO_user_init.PORT_Pin = (PORT_Pin_5 | PORT_Pin_6);
  PORT_Init(MDR_PORTB, &GPIO_user_init);

  GPIO_user_init.PORT_Pin = (PORT_Pin_2);
  PORT_Init(MDR_PORTC, &GPIO_user_init);

  GPIO_user_init.PORT_Pin = (PORT_Pin_1 | PORT_Pin_3);
  PORT_Init(MDR_PORTE, &GPIO_user_init);
}

/* Определение "кода" по нажатым кнопкам */
KeyCode joystick_get_key(void) {
  uint32_t i, sKey;
  static uint32_t _js_data[5];

  // Сдвигаем все буферы антидребезга на 1 влево
  for (i = 0; i < 5; i++)
    _js_data[i] = (_js_data[i] << 1);

  // Собираем данные с кнопок в массив с инверсией  значений (1 - нажата, 0 - не нажата)
  if (!(PORT_ReadInputDataBit(MDR_PORTC, PORT_Pin_2)))
    _js_data[0] = (_js_data[0] |= 0x1UL); /* SEL      PC2*/
  if (!(PORT_ReadInputDataBit(MDR_PORTB, PORT_Pin_6)))
    _js_data[1] = (_js_data[1] |= 0x1UL); /* RIGHT    PB6*/
  if (!(PORT_ReadInputDataBit(MDR_PORTE, PORT_Pin_3)))
    _js_data[2] = (_js_data[2] |= 0x1UL); /* LEFT     PE3*/
  if (!(PORT_ReadInputDataBit(MDR_PORTB, PORT_Pin_5)))
    _js_data[3] = (_js_data[3] |= 0x1UL); /* UP       PB5*/
  if (!(PORT_ReadInputDataBit(MDR_PORTE, PORT_Pin_1)))
    _js_data[4] = (_js_data[4] |= 0x1UL); /* DOWN     PE1*/

  // Устроняем дребезг
  static const uint32_t _jitter_mask = 0x1F;
  uint32_t actual_data[5];
  for (i = 0; i < 5; i++) {
    if (_js_data[i] == _jitter_mask)
      actual_data[i] = 1;
    else
      actual_data[i] = 0;
  }

  // Суммируем состояния кнопок
  sKey = 0;
  for (i = 0; i < 5; i++)
    sKey = sKey + actual_data[i];
  if (sKey == 0)
    return NOKEY;/* NOKEY */
  else if (sKey > 1)
    return MULTIPLE;/* MULTIPLE */
  else {
    for (i = 0; i < 5; i++)
      if (actual_data[i] == 1)
        return ((KeyCode)(i + 1)); // Если нажата только одна кнопка, то распознаем её
  }
  return NOKEY;
}
