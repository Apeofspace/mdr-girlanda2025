#include "main.h"
#include "MDR32F9Qx_dma.h"
#include "MDR32F9Qx_ssp.h"
#include "MDR32Fx.h"
#include "algos.h"
#include "joystick.h"
#include "delay.h"
#include <string.h>

pixel_t pixels[LEDS_NUMBER];
uint8_t tx_arr[LEDS_NUMBER * 3 * 8];
global_state_t state = {
  .ms = 0,
  .last_ms = 0,
  .speed = 0.5,
  .brightness = 0.5,
  .algos = {
    .count = 0,
    .selected = 0,
  },
  .flags = {
    .tx_in_progress = false,
    .paused = false,
  },
};

DMA_ChannelInitTypeDef DMA_InitStr = {
  .DMA_Priority = DMA_Priority_Default,
  .DMA_UseBurst = DMA_BurstClear,
  .DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY,
};

DMA_CtrlDataInitTypeDef DMA_TX_PriCtrlStr = {
  .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
  .DMA_Mode = DMA_Mode_Basic,
  .DMA_NumContinuous = DMA_Transfers_1,
  .DMA_SourceProtCtrl = DMA_SourcePrivileged,
  .DMA_DestProtCtrl = DMA_DestPrivileged,
  .DMA_DestBaseAddr = (uint32_t)(&(MDR_SSP2->DR)),
  .DMA_SourceIncSize = DMA_SourceIncByte,
  .DMA_DestIncSize = DMA_DestIncNo,
};

static void init_CPU() {
//attempts HSE
// оисциллятор стоит 8 мГц, аккуратно с множителем
  MDR_RST_CLK->HS_CONTROL = RST_CLK_HSE_ON; //Вкл. HSE
  if (RST_CLK_HSEstatus() == SUCCESS) {
    MDR_RST_CLK->CPU_CLOCK = (2 << 0); // set HSE
    MDR_RST_CLK->CPU_CLOCK |= (0 << 4); // set c3 to c2/1
    MDR_RST_CLK->PLL_CONTROL |= (9 << 8); // 9+1 multiplier for PLL_CPU
    // NOTE make sure that HSE_Value is set up correctly
    MDR_RST_CLK->PLL_CONTROL |= (1 << 2); // enable PLL_CPU
    MDR_RST_CLK->CPU_CLOCK |= (1 << 2); // set c2 to PLL_CPU
    MDR_RST_CLK->CPU_CLOCK |= (1 << 8); // set HCLK to c3
  } else {
// HSE failed, try HSI
    MDR_RST_CLK->HS_CONTROL = 0; //HSE OFF
    MDR_RST_CLK->CPU_CLOCK = 0 << 8; //HCLK -> HSI
    MDR_RST_CLK->CPU_CLOCK |= 0 << 2; //c2 -> c1
    MDR_RST_CLK->CPU_CLOCK |= 0 << 0; //c1 -> HSI
    MDR_RST_CLK->PLL_CONTROL = 1 << 2; //CPUPLL ON
    MDR_RST_CLK->PLL_CONTROL |= 9 << 8; //PLL multiplier = 9+1
    MDR_RST_CLK->CPU_CLOCK |= 1 << 2; //c2 -> CPUPLL
    MDR_RST_CLK->CPU_CLOCK |= 0 << 4; //c3 divider = 0
    MDR_RST_CLK->CPU_CLOCK |= 1 << 8; //HCLK -> c3
  }
}

static void init_SPI() {
  SSP_InitTypeDef SPI_init_struct;
  PORT_InitTypeDef GPIOInitStruct;

  // Включение тактирования
  RST_CLK_PCLKcmd(RST_CLK_PCLK_SSP2, ENABLE);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTD, ENABLE);

  // инициализация пинов
  GPIOInitStruct.PORT_PULL_UP = PORT_PULL_UP_OFF;
  GPIOInitStruct.PORT_PULL_DOWN = PORT_PULL_DOWN_ON;
  GPIOInitStruct.PORT_PD_SHM = PORT_PD_SHM_OFF;
  GPIOInitStruct.PORT_PD = PORT_PD_DRIVER;
  GPIOInitStruct.PORT_GFEN = PORT_GFEN_OFF;
  GPIOInitStruct.PORT_SPEED = PORT_SPEED_MAXFAST;
  GPIOInitStruct.PORT_MODE = PORT_MODE_DIGITAL;
  GPIOInitStruct.PORT_FUNC = PORT_FUNC_ALTER;
  GPIOInitStruct.PORT_Pin = (PORT_Pin_5 | PORT_Pin_6);
  PORT_Init (MDR_PORTD, &GPIOInitStruct);

  // инициализация SPI
  SSP_DeInit(MDR_SSP2);
  SSP_BRGInit(MDR_SSP2, SSP_HCLKdiv2); //40МГц
  SSP_StructInit (&SPI_init_struct);
  SPI_init_struct.SSP_SCR = 2; //второй делитель F_SSPCLK / ( CPSDVR * (1 + SCR) )
  SPI_init_struct.SSP_CPSDVSR = 2; // третий делитель (четное число)
  SPI_init_struct.SSP_Mode = SSP_ModeMaster;
  SPI_init_struct.SSP_WordLength = SSP_WordLength8b;
  SPI_init_struct.SSP_FRF = SSP_FRF_SPI_Motorola; //режим ssi или spi
  SPI_init_struct.SSP_HardwareFlowControl = SSP_HardwareFlowControl_None;
  SSP_Init(MDR_SSP2, &SPI_init_struct);

  // DMA
  RST_CLK_PCLKcmd(RST_CLK_PCLK_SSP1 | RST_CLK_PCLK_SSP2 | RST_CLK_PCLK_DMA, ENABLE);
  /* Disable all DMA request */
  MDR_DMA->CHNL_REQ_MASK_CLR = 0xFFFFFFFF;
  MDR_DMA->CHNL_USEBURST_CLR = 0xFFFFFFFF;
  DMA_DeInit();
  NVIC_ClearPendingIRQ(DMA_IRQn);
  NVIC_EnableIRQ(DMA_IRQn);

  SSP_Cmd(MDR_SSP2, ENABLE);
}

static void convert_pixels_for_spi(pixel_t* pix, uint8_t* result) {
  /* Превращаю каждый битик в отдельный байтик, который кидается на SPI.
     ноликам соответствует 0C, единицам 0F. */
  uint8_t temp;
  for (uint16_t led_ind = 0; led_ind < LEDS_NUMBER; led_ind++) {
    for (uint8_t color_ind = 0; color_ind < 3; color_ind++) {
      temp = *(((uint8_t*)(&pix[led_ind])) + color_ind);
      int j = 7;
      while (j >= 0) {
        *result = ((temp >> j) & 0x1) ? ONE : ZERO;
        result++;
        j--;
      }
    }
  }
}

void send_pixels() {
  convert_pixels_for_spi(pixels, tx_arr);

  // non DMA
  for (uint32_t i = 0; i < LEDS_NUMBER * 24; i++) {
    while (state.flags.tx_in_progress) {}; // ждём, если надо
    while (!(MDR_SSP2->SR & SSP_FLAG_TFE)) {};
    SSP_SendData(MDR_SSP2, tx_arr[i]);
    state.flags.tx_in_progress = false;
  }

  // DMA (doesn't work properly because of transaction size)

  // state.tx_in_progress = true;
  // DMA_TX_PriCtrlStr.DMA_CycleSize = sizeof(tx_arr);
  // DMA_TX_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)tx_arr;
  // DMA_InitStr.DMA_PriCtrlData = &DMA_TX_PriCtrlStr;
  // DMA_Init(DMA_Channel_SSP2_TX, &DMA_InitStr);
  // SSP_DMACmd(MDR_SSP2, SSP_DMA_TXE, ENABLE);

}

void DMA_IRQHandler(void) {
  SSP_DMACmd(MDR_SSP2, SSP_DMA_TXE, DISABLE);
  DMA_Cmd(DMA_Channel_SSP2_TX, DISABLE);
  state.flags.tx_in_progress = false;
}

static void register_alg(void func(pixel_t *pix)) {
  if (state.algos.count < MAX_ALGOS) {
    state.algos.funcs[state.algos.count] = func;
    state.algos.count++;
  }
}

static void joystick_loop() {
  switch (joystick_get_key()) {
  case SEL:
    state.flags.paused = !(state.flags.paused);
    break;
  case RIGHT:
    state.algos.selected++;
    if (state.algos.selected >= state.algos.count)
      state.algos.selected = 0;
    break;
  case LEFT:
    if (state.algos.selected == 0) {
      state.algos.selected = state.algos.count - 1;
    } else {
      state.algos.selected--;
    }
    break;
  case UP:
    state.speed = (state.speed >= 0.85) ? 1 : state.speed + 0.1;
    break;
  case DOWN:
    state.speed = (state.speed <= 0.25) ? 0.1 : state.speed - 0.1;
    break;
  case NOKEY:
  default:
    break;
  }
}

static void main_loop() {
  // программный таймер
  const static uint32_t main_loop_period_ms = 50;
  static uint32_t t0_main_loop = 0;
  uint32_t time_elapsed = GetMs() - t0_main_loop;
  if (time_elapsed < main_loop_period_ms)
    return;
  t0_main_loop = GetMs();

  if (!(state.flags.paused) && (state.algos.count > 0)) {
    state.ms += main_loop_period_ms; // инкрементировать время
    state.algos.funcs[state.algos.selected](pixels); // вызов функции генерации
    state.last_ms = state.ms;
    send_pixels(); // отправка на гирлянду
  }
}

int main() {
  // задержка, чтобы контроллер успел зайти в режим отладки, если программист
  // рукожоп и закирпичил миландр
  for (uint32_t del = 0 ; del < 1000000; del++) {
    __NOP();
  }

  init_CPU();
  init_SPI();
  init_joystick();
  init_SysTick();

  memset(pixels, 0x1C, sizeof(pixels));

  /* !!Регистрация алгоритмов!! */
  register_alg(lenochka);
  register_alg(breath_colors_rgb_table);
  register_alg(running_red_dot);

  while (1) {
    joystick_loop();
    main_loop();
  }
  return 0;
}
