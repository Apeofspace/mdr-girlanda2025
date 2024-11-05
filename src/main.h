#ifndef __MAIN_H
#define __MAIN_H

#include "MDR32Fx.h"
#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_dma.h"
#include <stdint.h>
// #include "MDR32F9Qx_timer.h"

#define LEDS_NUMBER 200
#define ONE 0xF0
#define ZERO 0xC0
#define MAX_ALGOS 20 // добавить, если не хватает
#define MAX_BRIGHTNESS 100 // ограничить яркость

typedef struct __packed__ {
  // It's blue, red, green in this order!
  uint8_t blue;
  uint8_t red;
  uint8_t green;
} pixel_t;

typedef struct {
  uint32_t ms;
  uint8_t speed;
  uint8_t brightness;
  bool paused;
  bool tx_in_progress;
  uint16_t num_reg_algos;
  uint16_t selected_algo_index;
  void (*algos[MAX_ALGOS])(pixel_t *pix);
} global_state_t;

extern pixel_t pixels[LEDS_NUMBER];
extern uint8_t tx_arr[LEDS_NUMBER * 3 * 8];
extern global_state_t state;

void DMA_IRQHandler(void);

#endif
