#ifndef INIT_H_INC
#define INIT_H_INC

#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi1;
extern RNG_HandleTypeDef hrng;

void sc4_hsm_init();

#endif
