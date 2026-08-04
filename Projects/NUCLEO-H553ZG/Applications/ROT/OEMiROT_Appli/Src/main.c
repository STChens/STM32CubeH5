/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main application file.
  *          This application demonstrates Firmware Update, protections
  *          and crypto testing functionalities.
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

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "com.h"
#include "common.h"
#include "main.h"
#include "low_level_flash.h"
#include "loader.h"

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif /* defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) */

/** @addtogroup USER_APP User App Example
  * @{
  */


/** @addtogroup USER_APP_COMMON Common
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

extern ARM_DRIVER_FLASH FLASH_PRIMARY_DATA_NONSECURE_DEV_NAME;
extern ARM_DRIVER_FLASH FLASH_PRIMARY_NONSECURE_DEV_NAME;

#define BOOTLOADER_BASE_NS              0x0BFA4000U

#ifdef NS_DATA_IMAGE_EN
#define NS_DATA_PRIMARY_OFFSET          (FLASH_BASE_NS + FLASH_AREA_5_OFFSET)
#define NS_DATA_IMAGE_DATA1_SIZE        32U
#define BL2_DATA_HEADER_SIZE            0x20
#endif

/* Enable print of boot time (obtained through DWT).
   DWT usage requires product state is not closed/locked.
   OEMxRoT logs must be disabled for relevant boot time. */
/* #define PRINT_BOOT_TIME */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t *pUserAppId;
const uint8_t UserAppId = 'A';
uint64_t time;
uint32_t end;

/* Private function prototypes -----------------------------------------------*/
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
#if !defined(MCUBOOT_OVERWRITE_ONLY)
static void FW_Valid_SecureAppImage(void);
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) */
#endif /* (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if !defined(MCUBOOT_OVERWRITE_ONLY)
static void FW_Valid_AppImage(void);
#endif /* defined(MCUBOOT_OVERWRITE_ONLY) */
#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
#if !defined(MCUBOOT_OVERWRITE_ONLY)
static void FW_Valid_SecureDataImage(void);
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) */
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
#if (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
#if !defined(MCUBOOT_OVERWRITE_ONLY)
static void FW_Valid_NonSecureDataImage(void);
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) */
#endif /* (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1) */

const uint32_t MagicTrailerValue[] =
{
  0xf395c277,
  0x7fefd260,
  0x0f505235,
  0x8079b62c,
};
#if 0
static void SystemClock_Config(void);
#endif
void FW_APP_PrintMainMenu(void);
void FW_APP_Run(void);
void LOADER_Run(int8_t is_sysbl);
static void MX_GTZC_Init(void);

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
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(int argc, char **argv)
/*int main(void) */
{
  /* Get boot cycles */
  end = DWT->CYCCNT;

 /*
  * When OEMIROT_FAST_WAKE_UP is enabled, the OEMiRoT relies on the hardware SBF (standby flag)
  * to skip firmware image verification. The SBF flag remains set when the application is entered
  * after a wake-up from standby mode. This allows the application to handle its own software
  * context restoration. To maintain a secure execution environment, the user application must
  * clear the SBF flag after it is processed.
  *
  * In this example, the SBF flag is simply cleared without any processing.
  */
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SBF);
	
  /* SAU/IDAU, FPU and interrupts secure/non-secure allocation setup done */
  /* in SystemInit() based on partition_stm32h553xx.h file's definitions. */


  /* Enable SecureFault handler (HardFault is default) */
  SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk;

  /* STM32H5xx **SECURE** HAL library initialization:
       - Secure Systick timer is configured by default as source of time base,
         but user can eventually implement his proper time base source (a general
         purpose timer for example or other time source), keeping in mind that
         Time base duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined
         and handled in milliseconds basis.
       - Low Level Initialization
     */


  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* GTZC initialisation */
  MX_GTZC_Init();

  /* !!! To boot in a secure way, the RoT has configured and activated the Memory Protection Unit
      In order to keep a secure environment execution, you should reconfigure the
      MPU to make it compatible with your application
      In this example, MPU is disabled */
  HAL_MPU_Disable();

  /* All IOs are by default allocated to secure */
  /* Release them all to non-secure except PC.07 (LED1) kept as secure */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOF, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOG, GPIO_PIN_ALL, GPIO_PIN_NSEC);
  HAL_GPIO_ConfigPinAttributes(GPIOH, GPIO_PIN_ALL, GPIO_PIN_NSEC);

  /* STM32U5xx HAL library initialization:
  - Systick timer is configured by default as source of time base, but user
  can eventually implement his proper time base source (a general purpose
  timer for example or other time source), keeping in mind that Time base
  duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
  handled in milliseconds basis.
  - Set NVIC Group Priority to 3
  - Low Level Initialization
  */
  HAL_Init();  

  /* Get Boot Time */
  time = ((uint64_t)(end) * 1000U / SystemCoreClock);

  /* DeInitialize RCC to allow PLL reconfiguration when configuring system clock */
  //HAL_RCC_DeInit();

  /* Configure the system clock */
  //SystemClock_Config();

  /*  set example to const : this const changes in binary without rebuild */
  pUserAppId = (uint8_t *)&UserAppId;
	
  /* Configure Communication module */
  COM_Init();

  printf("\r\n======================================================================");
  printf("\r\n=              (C) COPYRIGHT 2023 STMicroelectronics                 =");
  printf("\r\n=                Built on %s %s                       =", __DATE__, __TIME__);
  printf("\r\n=                          User App #%c                               =", *pUserAppId);
  printf("\r\n======================================================================");
  printf("\r\n\r\n");

  /* User App firmware runs*/
  FW_APP_Run();

  while (1U)
  {}

}

/**
  * @brief GTZC Initialization Function
  * @param None
  * @retval None
  */
static void MX_GTZC_Init(void)
{

  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_ICACHE_REG,
                                           GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Display the TEST Main Menu choices on HyperTerminal
  * @param  None.
  * @retval None.
  */
void FW_APP_PrintMainMenu(void)
{
  printf("\r\n=================== Main Menu ============================\r\n\n");
	printf("  Start User Loader -------------------------------------- 0\r\n\n");
#if !defined MCUBOOT_PRIMARY_ONLY	
  printf("  Start System BootLoader -------------------------------- 1\r\n\n");
#endif
#ifdef NS_DATA_IMAGE_EN
  printf("  Display Non secure Data  ------------------------------ 2\r\n\n");
#endif
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 2)
  printf("  Validate Secure App Image ----------------------------- 3\r\n\n");
  printf("  Validate NonSecure App Image -------------------------- 4\r\n\n");
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 1)
  printf("  Validate App Image ------------------------------------ 3\r\n\n");
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 1) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
  printf("  Validate Secure Data Image ---------------------------- 5\r\n\n");
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
  printf("  Validate NonSecure Data Image ------------------------- 6\r\n\n");
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
  printf("  Selection :\r\n\n");
}

/**
  * @brief  Display the TEST Main Menu choices on HyperTerminal
  * @param  None.
  * @retval None.
  */
void FW_APP_Run(void)
{
  uint8_t key = 0U;

  /*##1- Print Main Menu message*/
  FW_APP_PrintMainMenu();

  while (1U)
  {
    /* Clean the input path */
    COM_Flush();

    /* Receive key */
    if (COM_Receive(&key, 1U, RX_TIMEOUT) == HAL_OK)
    {
      switch (key)
      {
				case '0' : 
					LOADER_Run(0);
					break;
        case '1' :
          LOADER_Run(1);
          break;
#ifdef NS_DATA_IMAGE_EN
        case '2' :
          NS_DATA_Display();
          break;
#endif
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 2)
        case '3':
          FW_Valid_SecureAppImage();
          break;
        case '4':
          FW_Valid_AppImage();
          break;
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 1)
        case '3':
          FW_Valid_AppImage();
          break;
#endif /* defined(MCUBOOT_OVERWRITE_ONLY) && defined(MCUBOOT_APP_IMAGE_NUMBER == 1) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
        case '5':
          FW_Valid_SecureDataImage();
          break;
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
        case '6':
          FW_Valid_NonSecureDataImage();
          break;
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1) */
        default:
          printf("Invalid Number !\r");
          break;
      }

      /* Print Main Menu message */
      FW_APP_PrintMainMenu();
    }
  }
}


/**
  * @brief  Perform Jump to the BootLoader
	* @param  is_sysbl 1: jump to system bootloader, else jump to user loader.
  * @retval None.
  */
void LOADER_Run(int8_t is_sysbl)
{
	if ( is_sysbl == 1 )
	{
		printf("\r\n  Start config before jumping to the bootloader");

		for (int i = 0; i < 16; i++)
		{
			/*SRAM1 -> MPCBB1*/
			GTZC_MPCBB1_NS->SECCFGR[i] = 0;
		}

		/*  change stack limit  */
		__set_MSPLIM(0);

		printf("\r\n  Standard Bootloader started");
		printf("\r\n  If you want to connect through USART interface, disconnect your TeraTerm");
		printf("\r\n  Start download with STM32CubeProgrammer through supported interfaces (USART/SPI/I2C/USB)\r\n");
		printf("\r\n");

		SECURE_sysloader_run();
	}
	else
	{
		SECURE_userloader_run();
	}
}

#if  !defined(MCUBOOT_OVERWRITE_ONLY)
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
static void FW_Valid_SecureAppImage(void)
{
    SECURE_ConfirmSecureAppImage();
    printf("  -- Secure App Firmware Confirm Done\r\n\n");
}
#endif /* MCUBOOT_APP_IMAGE_NUMBER == 2 */

#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
static void FW_Valid_SecureDataImage(void)
{
    SECURE_ConfirmSecureDataImage();
    printf("  -- Secure Data Firmware Confirm Done\r\n\n");
}
#endif /* MCUBOOT_S_DATA_IMAGE_NUMBER == 1 */

#if (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
/**
  * @brief  Write Confirm Flag for  :
  * @brief  - NonSecure Data image
  * @param  None
  * @retval None
  */
static void FW_Valid_NonSecureDataImage(void)
{
  const uint8_t FlagPattern[]={0x1 ,0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff };
  const uint32_t ConfirmAddress = FLASH_AREA_5_OFFSET  + FLASH_AREA_5_SIZE - (sizeof(MagicTrailerValue) + sizeof(FlagPattern));
  if (FLASH_PRIMARY_DATA_NONSECURE_DEV_NAME.ProgramData(ConfirmAddress, FlagPattern, sizeof(FlagPattern)) == ARM_DRIVER_OK)
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
#endif /* MCUBOOT_NS_DATA_IMAGE_NUMBER == 1 */

static void FW_Valid_AppImage(void)
{
  const uint8_t FlagPattern[]={0x1 ,0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff , 0xff, 0xff, 0xff };
#if (MCUBOOT_APP_IMAGE_NUMBER == 1)
 const uint32_t ConfirmAddress = FLASH_AREA_0_OFFSET  + FLASH_PARTITION_SIZE - (sizeof(MagicTrailerValue) + sizeof(FlagPattern));
#else
  const uint32_t ConfirmAddress = FLASH_AREA_1_OFFSET + FLASH_NS_PARTITION_SIZE - (sizeof(MagicTrailerValue) + sizeof(FlagPattern));
#endif
  if (FLASH_PRIMARY_NONSECURE_DEV_NAME.ProgramData(ConfirmAddress, FlagPattern, sizeof(FlagPattern)) == ARM_DRIVER_OK)
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


/**
  * @brief  Display the Data on HyperTerminal
  * @param  None.
  * @retval None.
  */
#ifdef NS_DATA_IMAGE_EN
void NS_DATA_Display(void)
{
  uint8_t *data1;
  data1 = (uint8_t*)(NS_DATA_PRIMARY_OFFSET+BL2_DATA_HEADER_SIZE);

  printf("  -- NS Data: %08x%08x..%08x%08x\r\n\n",
               *((int *)(&data1[0])),
               *((int *)(&data1[4])),
               *((int *)(&data1[NS_DATA_IMAGE_DATA1_SIZE - 8])),
               *((int *)(&data1[NS_DATA_IMAGE_DATA1_SIZE - 4]))
              );
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
  while (1U)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */
