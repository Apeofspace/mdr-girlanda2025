#include "delay.h"

volatile uint32_t msSinceStart = 0;
bool SysTickInitialized = false;

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

