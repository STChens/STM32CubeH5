/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main application file.
  *          This application demonstrates Secure Services
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "low_level_flash.h"
#include "appli_flash_layout.h"
#include "com.h"
#include "common.h"
#include "fw_update_app.h"

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif /* defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) */


/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/
#define APP_DATA_SIZE 32

extern ARM_DRIVER_FLASH FLASH_PRIMARY_DATA_SECURE_DEV_NAME;
extern ARM_DRIVER_FLASH FLASH_PRIMARY_SECURE_DEV_NAME;

/* Enable print of boot time (obtained through DWT).
   DWT usage requires product state is not closed/locked.
   OEMxRoT logs must be disabled for relevant boot time. */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if !defined(MCUBOOT_OVERWRITE_ONLY)
static void FW_Valid_AppImage(void);
#endif /* defined(MCUBOOT_OVERWRITE_ONLY) */
#if (MCUBOOT_DATA_IMAGE_NUMBER == 1)
#if !defined(MCUBOOT_OVERWRITE_ONLY)
static void FW_Valid_DataImage(void);
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_DATA_IMAGE_NUMBER == 1) */
#endif /* (MCUBOOT_DATA_IMAGE_NUMBER == 1) */

static void SystemClock_Config(void);
void Loader_PrintMainMenu(void);
void Loader_Run(void);

/* Callbacks prototypes */
void Error_Handler(void);

#if defined(__ICCARM__)
#include <LowLevelIOInterface.h>
#endif /* __ICCARM__ */

#if defined(__ICCARM__)
/* New definition from EWARM V9, compatible with EWARM8 */
int iar_fputc(int ch);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)
#elif defined ( __CC_ARM ) || defined(__ARMCC_VERSION)
/* ARM Compiler 5/6*/
int io_putchar(int ch);
#define PUTCHAR_PROTOTYPE int io_putchar(int ch)
#elif defined(__GNUC__)
#define PUTCHAR_PROTOTYPE int32_t uart_putc(int32_t ch)
#endif /* __ICCARM__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  */
PUTCHAR_PROTOTYPE
{
  COM_Transmit((uint8_t*)&ch, 1, TX_TIMEOUT);
  return ch;
}

/* Redirects printf to DRIVER_STDIO in case of ARMCLANG*/
#if defined(__ARMCC_VERSION)
FILE __stdout;

/* __ARMCC_VERSION is only defined starting from Arm compiler version 6 */
int fputc(int ch, FILE *f)
{
  /* Send byte to USART */
  io_putchar(ch);

  /* Return character written */
  return ch;
}
#elif defined(__GNUC__)
/* Redirects printf to DRIVER_STDIO in case of GNUARM */
int _write(int fd, char *str, int len)
{
  int i;

  for (i = 0; i < len; i++)
  {
    /* Send byte to USART */
    uart_putc(str[i]);
  }

  /* Return the number of characters written */
  return len;
}
#elif defined(__ICCARM__)
size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;

  for (idx = 0; idx < len; idx++)
  {
    iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
}
#endif /*  __GNUC__ */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
/*int main(void) */
{
  /* Disable MPU or reconfigure MPU according to App needs */
  //MPU->CTRL &= 0xFFFFFFFE; 

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* !!! To boot in a secure way, the RoT has configured and activated the Memory Protection Unit
      In order to keep a secure environment execution, you should reconfigure the
      MPU to make it compatible with your application
      In this example, MPU is disabled */
  HAL_MPU_Disable();

  /* DeInitialize RCC to allow PLL reconfiguration when configuring system clock */
  HAL_RCC_DeInit();

  /* Configure the system clock */
  SystemClock_Config();

  COM_Init();

  /* All IOs are by default allocated to secure */
#if defined(GPIOA)
  __HAL_RCC_GPIOA_CLK_ENABLE();
#endif /* GPIOA */
#if defined(GPIOB)
  __HAL_RCC_GPIOB_CLK_ENABLE();
#endif /* GPIOB */
#if defined(GPIOC)
  __HAL_RCC_GPIOC_CLK_ENABLE();
#endif /* GPIOC */
#if defined(GPIOD)
  __HAL_RCC_GPIOD_CLK_ENABLE();
#endif /* GPIOD */
#if defined(GPIOE)
  __HAL_RCC_GPIOE_CLK_ENABLE();
#endif /* GPIOE */
#if defined(GPIOG)
  __HAL_RCC_GPIOG_CLK_ENABLE();
#endif /* GPIOG */
#if defined(GPIOH)
  __HAL_RCC_GPIOH_CLK_ENABLE();
#endif /* GPIOH */

  printf("\r\n======================================================================");
  printf("\r\n=              (C) COPYRIGHT 2025 STMicroelectronics                 =");
  printf("\r\n=                                                                    =");
  printf("\r\n=                      OEMiROT Loader                                =");
  printf("\r\n======================================================================");
  printf("\r\n\r\n");

  /* User App firmware runs*/
  Loader_Run();

  while (1)
  {

  }
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (HSE BYPASS)
  *            SYSCLK(Hz)                     = 250000000  (CPU Clock)
  *            HCLK(Hz)                       = 250000000  (Bus matrix and AHBs Clock)
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1 (APB1 Clock  250MHz)
  *            APB2 Prescaler                 = 1 (APB2 Clock  250MHz)
  *            APB3 Prescaler                 = 1 (APB3 Clock  250MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 100
  *            PLL_P                          = 2
  *            PLL_Q                          = 2
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* The voltage scaling allows optimizing the power consumption when the device is
  clocked below the maximum system frequency, to update the voltage scaling value
  regarding system frequency refer to product datasheet.
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Use HSE in bypass mode and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS_DIGITAL;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 250;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
  /* Select PLL as system clock source and configure the HCLK, PCLK1, PCLK2 and PCLK3
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2| RCC_CLOCKTYPE_PCLK3);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }

  /** Configure the programming delay */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/**
  * @brief  Display the TEST Main Menu choices on HyperTerminal
  * @param  None.
  * @retval None.
  */
void Loader_PrintMainMenu(void)
{
  printf("\r\n=================== Main Menu ============================\r\n\n");
  printf("  App FW/Data Update ------------------------------------ 1\r\n\n");
#if !defined(MCUBOOT_OVERWRITE_ONLY) 
  printf("  Validate App Image ------------------------------------ 2\r\n\n");
#if (MCUBOOT_DATA_IMAGE_NUMBER == 1)
  printf("  Validate Data Image ----------------------------------- 3\r\n\n");
#endif /* (MCUBOOT_DATA_IMAGE_NUMBER == 1) */
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) */
  printf("  Selection :\r\n\n");
}

/**
  * @brief  Display the TEST Main Menu choices on HyperTerminal
  * @param  None.
  * @retval None.
  */
void Loader_Run(void)
{
  uint8_t key = 0U;

  /*##1- Print Main Menu message*/
  Loader_PrintMainMenu();

  while (1U)
  {
    /* Receive key */
    if (COM_Receive(&key, 1U, 1000) == HAL_OK)
    {
      switch (key)
      {
        case '1' :          
          FW_UPDATE_Run();
          break;
#if !defined(MCUBOOT_OVERWRITE_ONLY)
        case '2':
          FW_Valid_AppImage();
          break;
#endif /* defined(MCUBOOT_OVERWRITE_ONLY) && defined(MCUBOOT_APP_IMAGE_NUMBER == 1) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_DATA_IMAGE_NUMBER == 1)
        case '3':
          FW_Valid_DataImage();
          break;
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_DATA_IMAGE_NUMBER == 1) */
        default:
          printf("Invalid Number !\r");
          break;
      }

      /* Print Main Menu message */
      Loader_PrintMainMenu();
    }
  }
}


#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_DATA_IMAGE_NUMBER == 1)
/**
  * @brief  Secure Operation to confirm Secure Data Image.
  * @param  None
  * @param  None
  * @retval None
  */
static void FW_Valid_DataImage(void)
{
  const uint8_t FlagPattern[]={0x1 ,0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff };
  const uint32_t ConfirmAddress = FLASH_AREA_4_OFFSET  + FLASH_AREA_4_SIZE - (TRAILER_MAGIC_SIZE + sizeof(FlagPattern));

  if (FLASH_PRIMARY_DATA_SECURE_DEV_NAME.ProgramData(ConfirmAddress, FlagPattern, sizeof(FlagPattern)) == ARM_DRIVER_OK)
  {
    printf("  -- Data Firmware Confirm Done\r\n\n");
  }
  else
  {
    printf("  -- Confirm Flag Not Correctlty Written \r\n\n");
  }
}
#endif /* MCUBOOT_DATA_IMAGE_NUMBER == 1 */

#if !defined(MCUBOOT_OVERWRITE_ONLY)
/**
  * @brief  Secure Operation to confirm Secure App Image.
  * @param  None
  * @param  None
  * @retval None
  */
static void FW_Valid_AppImage(void)
{
  const uint8_t FlagPattern[]={0x1 ,0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff };
  const uint32_t ConfirmAddress = FLASH_AREA_0_OFFSET  + FLASH_AREA_0_SIZE - (TRAILER_MAGIC_SIZE + sizeof(FlagPattern));

  if (FLASH_PRIMARY_SECURE_DEV_NAME.ProgramData(ConfirmAddress, FlagPattern, sizeof(FlagPattern)) == ARM_DRIVER_OK)
  {
#if defined(__ARMCC_VERSION)
    printf("  --  Confirm Flag  correctly written %x %x \r\n\n",ConfirmAddress ,FlagPattern[0] );
#else
    printf("  --  Confirm Flag  correctly written %lx %x \r\n\n",ConfirmAddress , FlagPattern[0] );
#endif
  }
  else
  {
    printf("  -- Confirm Flag Not Correctlty Written \r\n\n");
  }
}
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) */

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)      

/**
  * @brief  Callback called by secure code following a secure fault interrupt
  * @note   This callback is called by secure code thanks to the registration
  *         done by the non-secure application with non-secure callable API
  *         SECURE_RegisterCallback(SECURE_FAULT_CB_ID, (void *)SecureFault_Callback);
  * @retval None
  */
void SecureFault_Callback(void)
{
  /* Go to error infinite loop when Secure fault generated by IDAU/SAU check */
  /* because of illegal access */
  Error_Handler();
}


/**
  * @brief  Callback called by secure code following a GTZC TZIC secure interrupt (GTZC_IRQn)
  * @note   This callback is called by secure code thanks to the registration
  *         done by the non-secure application with non-secure callable API
  *         SECURE_RegisterCallback(GTZC_ERROR_CB_ID, (void *)SecureError_Callback);
  * @retval None
  */
void SecureError_Callback(void)
{
  /* Go to error infinite loop when Secure error generated by GTZC check */
  /* because of illegal access */
  Error_Handler();
}
#endif

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  NVIC_SystemReset();
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }

}
#endif /* USE_FULL_ASSERT */
