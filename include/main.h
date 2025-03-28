
#ifndef MAIN_H 
#define MAIN_H 

#include "stm32f3xx_hal.h"

#define LEDS_GPIO_PORT GPIOE

void LedPinsInit();
void ButtonInit();
void NextGameStep();
void UserInputHandler();
void LightStateAll(uint8_t);
void LightGameWin();
void LightRoundWin();
void LightLose();
void LightWaitToStart();
void CheckGameSituation();
void GameCycle();
void LightByState();
void GameInit();
void GameRestart();

#endif