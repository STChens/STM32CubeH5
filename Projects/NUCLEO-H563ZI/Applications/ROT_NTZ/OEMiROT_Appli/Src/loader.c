/**
  ******************************************************************************
  * @file    loader.c
  * @author  MCD Application Team
  * @brief   Test Protections module.
  *          This file provides set of firmware functions to manage Test Protections
  *          functionalities.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023-2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include "stm32h5xx_hal.h"
#include "appli_flash_layout.h"
#include "stdio.h"

/* Private define ------------------------------------------------------------*/
/* Systeme Flash description */
#define BOOTLOADER_BASE_NS              (0x0BF97000)
#define LOADER_ADDRESS                  LOADER_CODE_START //(BOOTLOADER_BASE_NS)

/* Private function prototypes -----------------------------------------------*/
void LOADER_Run(void);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Display the TEST Main Menu choices on HyperTerminal
  * @param  Testnumber giving the test that generate a reset
  * @retval None.
  */
void LOADER_Run(void)
{  
#if (LOADER_CODE_START == 0)
  uint32_t boot_address = *(uint32_t *)(BOOTLOADER_BASE_NS + 4U);
  __set_MSP((*(uint32_t *)BOOTLOADER_BASE_NS));
  SCB->VTOR = BOOTLOADER_BASE_NS;
  
  printf("\r\n  Jump to system bootloader.\r\n");
  printf("  Disconnect UART console before connecting using STM32CubeProgrammer!\r\n"); 
#if (IMAGE_SECONDARY_PARTITION_OFFSET > 0x0)  
  printf("  Download the FW image binary to address 0x%08x\r\n", FLASH_BASE + IMAGE_SECONDARY_PARTITION_OFFSET);
#endif
#if (MCUBOOT_DATA_IMAGE_NUMBER == 1) && (DATA_IMAGE_SECONDARY_PARTITION_OFFSET > 0x0)
  printf("  Download the data image binary to address 0x%08x\r\n", FLASH_BASE + DATA_IMAGE_SECONDARY_PARTITION_OFFSET);
#endif  
#else  
  uint32_t boot_address = *(uint32_t *)(LOADER_ADDRESS + 4U);
  __set_MSP((*(uint32_t *)LOADER_ADDRESS));
  SCB->VTOR = LOADER_ADDRESS;
  
  printf("\r\n  Jump to OEMiROT_Loader\r\n");  
#endif
  
    __asm volatile("movs r0, %0\n"
               "movs r1, #0\n" /*clear registers before jumping to non-secure*/
               "movs r2, #0\n"
               "movs r3, #0\n"
               "movs r4, #0\n"
               "movs r5, #0\n"
               "movs r6, #0\n"
               "movs r7, #0\n"
               "mov r8, r5\n"
               "mov r9, r5\n"
               "mov r10, r5\n"
               "mov r11, r5\n"
               "mov r12, r5\n"
               "MSR APSR_nzcvq,r1\n" /*clear APSR*/
               "bx r0\n"::"r"(boot_address)); /*jump to non-secure address*/
  /*BX, no return here possible*/
}
