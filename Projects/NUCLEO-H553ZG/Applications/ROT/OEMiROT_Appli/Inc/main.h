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
#include "appli_flash_layout.h"

#define S_CODE_OFFSET      0x20000 /* This define is updated automatically from ROT_BOOT project */
#define NS_CODE_SIZE       0x50000 /* This define is updated automatically from ROT_BOOT project */
#define S_CODE_SIZE        0xBE000 /* This define is updated automatically from ROT_BOOT project */
#define NS_CODE_OFFSET     (S_CODE_OFFSET + S_CODE_SIZE) /* Non secure code Offset */
#define IMAGE_HEADER_SIZE  (0x400)  /* mcuboot headre size */
#define S_CODE_START       (FLASH_BASE_S + S_CODE_OFFSET + IMAGE_HEADER_SIZE)
#define LOADER_S_CODE_START 0x0C010000

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* User can use this section to tailor UARTx instance used and associated
   resources */


/* Exported macros -----------------------------------------------------------*/
#if defined MCUBOOT_PRIMARY_ONLY
#define USE_SYSTEM_LOADER               /* Defined: BL2 will allow system bootloader to write to primary slot */
#endif /* defined MCUBOOT_PRIMARY_ONLY */
/* Exported functions --------------------------------------------------------*/
void Error_Handler(void);
/* Exported variables --------------------------------------------------------*/
extern uint32_t TestNumber;
#endif /* MAIN_H */
