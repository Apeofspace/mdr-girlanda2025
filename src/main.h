#ifndef __MAIN_H
#define __MAIN_H

#include "MDR32Fx.h"
#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_dma.h"
#include <stdint.h>
// #include <stdbool.h>
#include "math.h"
// #include "MDR32F9Qx_timer.h"

#define LEDS_NUMBER 200
#define ONE 0xF0
#define ZERO 0xC0
#define MAX_ALGOS 20 // добавить, если не хватает

typedef struct __packed__ {
  // It's blue, red, green in this order!
  uint8_t blue;
  uint8_t red;
  uint8_t green;
} pixel_t;

typedef struct {
  uint32_t ms; // миллисекунд с начала работы (минус паузы)
  uint32_t last_ms; // время предыдущего вызова алгоритма
  float speed; // скорость 0..1 (по умолчанию 0.5)
  float brightness; // яркость 0..1 (по умолчанию 0.5)
  bool recently_switched_algo; // поменяли алгоритм только что
  struct {
    bool paused;
    bool tx_in_progress;
  } flags ;
  struct {
    uint16_t count;
    uint16_t selected;
    void (*funcs[MAX_ALGOS])(pixel_t *pix);
  } algos ;
} global_state_t;

/* Hacked class */
typedef struct {
  void (*init)();
  void (*algo)(pixel_t *pix);
  void* params_struct;
} algo_t;

extern pixel_t pixels[LEDS_NUMBER];
extern uint8_t tx_arr[LEDS_NUMBER * 3 * 8];
extern global_state_t state;

void DMA_IRQHandler(void);
void send_pixels();

#endif
