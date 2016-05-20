/*
 * pinmap.h
 *
 *  Created on: Jul 3, 2014
 *      Author: Nystrom Engineering
 */

#ifndef PINMAP_H_
#define PINMAP_H_


//LED pin definitions
#define LED_PORT		GPIOB

#define LED1_PIN		GPIO_PIN_14
#define LED2_PIN		GPIO_PIN_15

//LED pin definitions
#define LEDRG_PORT GPIOB
#define LEDB_PORT GPIOC

#define LEDR_PIN GPIO_PIN_14
#define LEDG_PIN GPIO_PIN_15
#define LEDB_PIN GPIO_PIN_13

//Tactile Switch pin definitions
#define SW_PORT		GPIOB

#define SW1_PIN		GPIO_PIN_12
#define SW2_PIN		GPIO_PIN_13

#define DISP_PORT GPIOA
#define DISP_RSTn_PIN GPIO_PIN_2
#define DISP_DCn_PIN GPIO_PIN_3
#define DISP_CSn_PIN GPIO_PIN_4

#endif /* PINMAP_H_ */
