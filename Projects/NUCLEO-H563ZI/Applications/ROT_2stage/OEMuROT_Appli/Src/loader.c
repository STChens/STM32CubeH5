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
#include "low_level_security.h"
#include "stdio.h"

/* Private define ------------------------------------------------------------*/
/* Systeme Flash description */
#define BOOTLOADER_BASE_NS              (0x0BF97000)
#if (LOADER_CODE_SIZE > 0 )    
#define LOADER_ADDRESS                  (LOADER_CODE_START)
#else
#define LOADER_ADDRESS                  (BOOTLOADER_BASE_NS)
#endif

/* Private function prototypes -----------------------------------------------*/
void LOADER_Run(void);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Display the TEST Main Menu choices on HyperTerminal
  * @param  Testnumber giving the test that generate a reset
  * @retval None.
  */
#if (LOADER_CODE_SIZE > 0 )    

void LOADER_Run(void)
{
  printf("\r\n  Jump to UART Ymodem loader");
  
  uint32_t boot_address = *(uint32_t *)(LOADER_ADDRESS + 4U);
  SCB->VTOR = LOADER_ADDRESS;
  
  // Disable MPU
  MPU->CTRL &= 0xFFFFFFFE;
  
  __set_MSP((*(uint32_t *)LOADER_ADDRESS));
  __DSB();
  __ISB();
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
#else
void LOADER_Run(void)
{
  printf("\r\n  Standard Bootloader started");
  printf("\r\n  If you want to connect through USART interface, disconnect your TeraTerm");
  printf("\r\n  Start download with STM32CubeProgrammer through supported interfaces (USART/SPI/I2C/USB)\r\n");

  printf("\r\n");

  pwr_loader_cfg();

  /* configure GTZC to allow non secure / privileged loader execution */
  gtzc_loader_cfg();

  /* configure SAU to allow non secure / privileged loader execution */
  sau_loader_cfg();

  /* Configure NVIC */
  nvic_loader_cfg();

  /* enable FPU */
  fpu_enable_cfg();

  /* Configure GPIO to non-secure */
  gpio_loader_cfg();

  /* Configure flash to non-secure */
  flash_loader_cfg();

  uint32_t boot_address = cmse_nsfptr_create(*(uint32_t *)(BOOTLOADER_BASE_NS + 4U));

  /*Increment HDPL to HDPL3*/
  SET_BIT(SBS->HDPLCR,  SBS_HDPLCR_INCR_HDPL);

  __TZ_set_MSP_NS((*(uint32_t *)BOOTLOADER_BASE_NS));
  SCB_NS->VTOR = BOOTLOADER_BASE_NS;


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
               "bxns r0\n"::"r"(boot_address)); /*jump to non-secure address*/
  /*BXNS, no return here possible*/
}
#endif