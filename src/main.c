#include "stm32f3xx_hal.h"
#include "main.h"

uint16_t led_gpios[] = {GPIO_PIN_9,  GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12,
                        GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15, GPIO_PIN_8};

#define WIN_GPIO_IND 0

#define LED_GPIOS_SIZE sizeof(led_gpios) / sizeof(uint16_t)

uint8_t led_gpios_state[LED_GPIOS_SIZE] = {0};

enum GameState {
  kWaitStart,
  kPlayingRound,
  kPlayerPressed,
  kRoundWin,
  kGameWin,
  kGameLose
};
enum GameState game_state;

#define DIFFICULTY_RANGE_START 6
#define DIFFICULTY_SPEED_START 50
#define DIFFICULTY_SPEED_END 30

uint8_t difficulty_range;
uint16_t difficulty_speed;

int main() {
  HAL_Init();
  LedPinsInit();
  ButtonInit();
  while (1) {
    GameInit();
    while (game_state == kWaitStart) {
      LightWaitToStart();
    }
    while (game_state != kGameWin && game_state != kGameLose) {
      GameCycle();
      LightStateAll(0);
      if (game_state == kRoundWin) {
        LightRoundWin();
        continue;
      } 
    }

    if (game_state == kGameWin) {
      LightGameWin();
    }
    if (game_state == kGameLose) {
      LightLose();
    }
    GameRestart();
  }
}

void GameInit() {
  game_state = kWaitStart;
  difficulty_range = DIFFICULTY_RANGE_START;
  difficulty_speed = DIFFICULTY_SPEED_START;
}

void GameRestart() { 
  GameInit(); 
}

void NextGameStep() {
  if (difficulty_range > 1) {
    difficulty_range--;
  } else {
    difficulty_speed = DIFFICULTY_SPEED_END;
  }
}

void UserInputHandler() {
  if (game_state == kWaitStart) {
    game_state = kPlayingRound;
  }
  else if (game_state == kPlayingRound) {
    game_state = kPlayerPressed;
  }
}

void LightOff() {
  for (uint8_t pin_ind = 0; pin_ind < LED_GPIOS_SIZE; pin_ind++) {
    HAL_GPIO_WritePin(LEDS_GPIO_PORT, led_gpios[pin_ind], 0);
  }
}

void LightStateAll(uint8_t enabled) {
  for (uint8_t pin_ind = 0; pin_ind < LED_GPIOS_SIZE; pin_ind++) {
    HAL_GPIO_WritePin(LEDS_GPIO_PORT, led_gpios[pin_ind], enabled);
    led_gpios_state[pin_ind] = enabled;
  }
}

void LightGameWin() {
  for (char i = 0; i < 4; i++) {
    LightStateAll(1);
    HAL_Delay(500);
    LightStateAll(0);
    HAL_Delay(500);
  }
}

void LightRoundWin() {
  for (char i = 0; i < 4; i++) {
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[2]);
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[3]);
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[6]);
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[7]);
    HAL_Delay(500);
  }
  LightStateAll(0);
}

void LightLose() {
  for (char i = 0; i < 4; i++) {
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[0]);
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[1]);
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[4]);
    HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[5]);
    HAL_Delay(500);
  }
  LightStateAll(0);
}

void LightWaitToStart() {
    for (uint8_t pin_ind = 0; pin_ind < LED_GPIOS_SIZE; pin_ind++) {
      HAL_GPIO_TogglePin(LEDS_GPIO_PORT, led_gpios[pin_ind]);
      HAL_Delay(100);
  }
}

void CheckGameSituation() {
  if (game_state != kPlayerPressed) return;
  LightOff();
  if (led_gpios_state[WIN_GPIO_IND]) {
    if (difficulty_speed == DIFFICULTY_SPEED_END && difficulty_range == 1) {
      game_state = kGameWin;
      return;
    }
    game_state = kRoundWin;
    NextGameStep();
  } else {
    game_state = kGameLose;
  }
}

void LightByState() {
  for (uint8_t pin_num = 0; pin_num < LED_GPIOS_SIZE; pin_num++) {
    HAL_GPIO_WritePin(LEDS_GPIO_PORT, led_gpios[pin_num],  led_gpios_state[pin_num]);
  }
}

void GameCycle() {
  uint8_t current_pos = 0;
  uint32_t future_time = 0;
  game_state = kPlayingRound;
  while (game_state == kPlayingRound) {
    LightStateAll(0);
    for (uint8_t pin_offset = 0; pin_offset < difficulty_range; pin_offset++) {
      if (current_pos + pin_offset < LED_GPIOS_SIZE) {
        led_gpios_state[current_pos + pin_offset] = 1;
      } else {
        led_gpios_state[(current_pos + pin_offset) - LED_GPIOS_SIZE] = 1;
      }
    }
    LightByState();
    future_time = HAL_GetTick() + DIFFICULTY_SPEED_START;
    while (HAL_GetTick() < future_time) {
      CheckGameSituation();
    }
    current_pos++;
    if (current_pos >= LED_GPIOS_SIZE) {
      current_pos = 0;
    }
  }
}

void EXTI0_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_pin) {
  if (GPIO_pin == GPIO_PIN_0) {
    UserInputHandler();
  }
}

void ButtonInit() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void LedPinsInit() {
  __HAL_RCC_GPIOE_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;

  for (uint8_t pin_ind = 0; pin_ind < LED_GPIOS_SIZE; pin_ind++) {
    GPIO_InitStruct.Pin = led_gpios[pin_ind];
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  }
}

void SysTick_Handler() { HAL_IncTick(); }