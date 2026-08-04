/**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MAIN_H
#define MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"
#include "com.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* User can use this section to tailor UARTx instance used and associated
   resources */


/* Exported macros -----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Error_Handler(void);

#ifdef USB_CDC_SUPPORT
void USB_ReceiveData_Handle(uint8_t *pBuf, uint32_t len);
#endif
/* Exported variables --------------------------------------------------------*/
extern uint32_t TestNumber;
#endif /* MAIN_H */
