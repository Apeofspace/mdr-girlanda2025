/*
Миландровская библиотека переписанная так, чтобы на неё не так страшно было смотреть
Ловит нажатия кнопочек на платочке от LDM
*/

#include "joystick.h"
#include "MDR32F9Qx_port.h"


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

