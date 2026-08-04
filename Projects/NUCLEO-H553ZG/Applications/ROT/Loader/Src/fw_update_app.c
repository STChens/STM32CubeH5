/**
  ******************************************************************************
  * @file    fw_update_app.c
  * @author  MCD Application Team
  * @brief   Firmware Update module.
  *          This file provides set of firmware functions to manage Firmware
  *          Update functionalities.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "stm32h5xx_hal.h"
#include "com.h"
#include "common.h"
#include "ymodem.h"
#include "fw_update_app.h"
#include "region_defs.h"
#include "Driver_Flash.h"
#include "string.h"

/** @addtogroup USER_APP User App Example
  * @{
  */
#define BOOTLOADER_BASE_NS                  (0x0BFA4000U)
#define BOOTLOADER_SIZE                     (0xC000U)
/* Engi bits */
#define ENGI_START                          (0x08FFF800UL)
//#define ENGI_SIZE                           (0x40UL)
#define ARRAY_SIZE(array)                   (sizeof(array) / sizeof((array)[0]))
/* SRAM1 configuration
   =================== */
/* SRAM1 NB super-block */
#define GTZC_MPCBB1_NB_VCTR (16U)
/* SRAM3 NB super-block */
#define GTZC_MPCBB3_NB_VCTR (20U)
/* MPCBB : All SRAM block non privileged + privileged */
#define GTZC_MPCBB_ALL_NPRIV (0x00000000UL)
/* MPCBB : All SRAM block non secure */
#define GTZC_MPCBB_ALL_NSEC (0x00000000UL)
#define TZSC_MASK_R1  (GTZC_CFGR1_USART2_Msk | GTZC_CFGR1_USART3_Msk | GTZC_CFGR1_SPI3_Msk  | GTZC_CFGR1_SPI2_Msk | \
                       GTZC_CFGR1_I2C1_Msk | GTZC_CFGR1_I3C1_Msk   | GTZC_CFGR1_IWDG_Msk )
#define TZSC_MASK_R2  (GTZC_CFGR2_USART1_Msk | GTZC_CFGR2_SPI1_Msk   | GTZC_CFGR2_I2C3_Msk  | GTZC_CFGR2_USB_Msk | \
                       GTZC_CFGR2_FDCAN2_Msk | GTZC_CFGR2_FDCAN1_Msk| GTZC_CFGR2_UCPD1_Msk )
#define TZSC_MASK_R3  (GTZC_CFGR3_ICACHE_REG_Msk | GTZC_CFGR3_CRC_Msk )

/* NVIC configuration
   ================== */
/** Interrupts 0 .. 31 */
/** in ITNS0 GPDMA1_Channel0_IRQn | GPDMA1_Channel1_IRQn | GPDMA1_Channel2_IRQn bit27|28|29  is non secure */
/** 0x3800000000 => (switch to binary format) : 0011 1000 0000 0000 0000 0000 0000 0000 */
/** From binary format we can check that bit positions 27, 28, 29 are setted */
#define RSS_NVIC_INIT_ITNS0_VAL      (0x38000000U)

/** Interrupts 32 .. 63 */
/** in ITNS1 no bit is non secure */
#define RSS_NVIC_INIT_ITNS1_VAL      (0x00000000U)

/** Interrupts 64 .. 95 */
/** in ITNS2 OTG_FS_IRQn 74, i.e bit 10 is non secure (1) */
#define RSS_NVIC_INIT_ITNS2_VAL      (0x00000400U)

/** Interrupts 96 .. 127 */
/** I3C1_EV_IRQn = 123 */ 
#define RSS_NVIC_INIT_ITNS3_VAL      (0x08000000U)

#define GPIOA_MASK_SECCFG    (GPIO_PIN_MASK & \
                              (GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 |  GPIO_PIN_8 | GPIO_PIN_9  | GPIO_PIN_10 | \
                               GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15 ))
#define GPIOB_MASK_SECCFG    (GPIO_PIN_MASK & \
                              (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 ))
#define GPIOC_MASK_SECCFG    (GPIO_PIN_MASK & (GPIO_PIN_1 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 ))
#define GPIOD_MASK_SECCFG    (GPIO_PIN_MASK & (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_13 ))

extern ARM_DRIVER_FLASH FLASH_PRIMARY_SECURE_DEV_NAME;
extern ARM_DRIVER_FLASH FLASH_PRIMARY_DATA_SECURE_DEV_NAME;

struct sau_cfg_t
{
  uint32_t RNR;
  uint32_t RBAR;
  uint32_t RLAR;
};
const struct sau_cfg_t sau_load_cfg[] =
{
  /* allow non secure access to SRAM3 */
  {
    0,
    (uint32_t)SRAM1_BASE_NS,
    ((uint32_t)SRAM3_BASE_NS + SRAM3_SIZE - 1U),
  },
  /* allow non secure access to periph */
  {
    1,
    (uint32_t)PERIPH_BASE_NS,
    ((uint32_t)PERIPH_BASE_S + 0xFFFFFFFUL),
  },
  /* allow non secure access to all user flash except secure part and area covered by HDP extension */
  {
    2,
    (uint32_t)FLASH_BASE_NS,
    (uint32_t)(FLASH_BASE_NS + FLASH_SIZE_DEFAULT - 1U),
  },
  /* allow non secure access to bootloader code */
  {
    3,
    (uint32_t)BOOTLOADER_BASE_NS,
    ((uint32_t)BOOTLOADER_BASE_NS + BOOTLOADER_SIZE - 1U),
  },
  /* allow non secure access to Engi bits */
  {
    4,
    (uint32_t)ENGI_START,
    ((uint32_t)ENGI_START + ENGI_SIZE - 1U),
  },
};

extern ARM_DRIVER_FLASH LOADER_FLASH_DEV_NAME;
/** @addtogroup  FW_UPDATE Firmware Update Example
  * @{
  */
	
/** @defgroup  FW_UPDATE_Private_Variables Private Variables
  * @{
  */
static uint32_t m_uFileSizeYmodem = 0U;    /* !< Ymodem File size*/
static uint32_t m_uNbrBlocksYmodem = 0U;   /* !< Ymodem Number of blocks*/
static uint32_t m_uPacketsReceived = 0U;   /* !< Ymodem packets received*/
static uint32_t m_uFlashSectorSize = 0U;   /* !< Flash Sector Size */
static uint32_t m_uFlashMinWriteSize = 0U; /* !< FLash Min Write access*/
#if   !defined(MCUBOOT_PRIMARY_ONLY)
/** @defgroup  FW_UPDATE_Private_Const Private Const
  * @{
  */
const uint32_t MagicTrailerValue[] =
{
  0xf395c277,
  0x7fefd260,
  0x0f505235,
  0x8079b62c,
};
#endif /* !defined(MCUBOOT_PRIMARY_ONLY) */

/**
  * @}
  */

/** @defgroup  FW_UPDATE_Private_Functions Private Functions
  * @{
  */
static void FW_UPDATE_PrintWelcome(void);
static HAL_StatusTypeDef FW_UPDATE_DownloadNewFirmware(SFU_FwImageFlashTypeDef *pFwImageDwlArea);
static HAL_StatusTypeDef FW_UPDATE_SECURE_APP_IMAGE(void);
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
static HAL_StatusTypeDef FW_UPDATE_NONSECURE_APP_IMAGE(void);
#endif /* (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
static HAL_StatusTypeDef FW_UPDATE_SECURE_DATA_IMAGE(void);
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
#if (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
static HAL_StatusTypeDef FW_UPDATE_NONSECURE_DATA_IMAGE(void);
#endif /* (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1) */

static void secure_internal_flash(uint32_t offset_start, uint32_t offset_end);
static void SECURE_loader_run(void);
static void LOADER_Run(void);

extern ARM_DRIVER_FLASH FLASH_DEV_NAME;
#define BANK_NUMBER  2

static void secure_internal_flash(uint32_t offset_start, uint32_t offset_end)
{
  volatile uint32_t *SecBB[4]= {&FLASH_S->SECBB1R1, &FLASH_S->SECBB1R2,
                                &FLASH_S->SECBB2R1, &FLASH_S->SECBB2R2};
  volatile uint32_t *ptr;
  uint32_t regwrite=0x0, index;
  uint32_t block_start = offset_start;
  uint32_t block_end =  offset_end;
  const ARM_FLASH_INFO *flash_info;

  flash_info = FLASH_DEV_NAME.GetInfo();

  block_start = block_start / flash_info->page_size;
  block_end = (block_end / flash_info->page_size) ;

  /* 1f is for 32 bits */
  for (index = block_start & ~0x1f; index < flash_info->sector_count ; index++)
  { /* clean register on index aligned */
    if (!(index & 0x1f)){
       regwrite=0x0;
    }
    if ((index >= block_start) && (index <= block_end))
      regwrite = regwrite | ( 1 << (index & 0x1f));
    /* write register when 32 sub block are set or last block to set  */
    if ((index & 0x1f ) == 0x1f) {
      ptr = (uint32_t *)SecBB[index>>5];
      *ptr = regwrite;
    }
  }
}

/**
  * @brief  Sau idau configuration before jumping into loader
  * @retval None
  */
void SECURE_loader_run(void)
{
  uint32_t i = 0U;

  /* configuration stage */
  __HAL_RCC_GTZC1_CLK_ENABLE();

  /* Allow secure to access to non secure */
  GTZC_MPCBB1_S->CR |= GTZC_MPCBB_CR_SRWILADIS_Msk;
  /* All bocks of SRAM1 configured non secure / privileged (default value) */
  for (i = 0; i < GTZC_MPCBB1_NB_VCTR; i++)
  {
    /*SRAM1 -> MPCBB1*/
    GTZC_MPCBB1_S->SECCFGR[i] = GTZC_MPCBB_ALL_NSEC;
    GTZC_MPCBB1_S->PRIVCFGR[i] = GTZC_MPCBB_ALL_NPRIV;
  }
  /* All bocks of SRAM3 configured non secure / privileged (default value) */
  for (i = 0; i < GTZC_MPCBB3_NB_VCTR; i++)
  {
    /*SRAM3 -> MPCBB3*/
    GTZC_MPCBB3_S->SECCFGR[i] = GTZC_MPCBB_ALL_NSEC;
    GTZC_MPCBB3_S->PRIVCFGR[i] = GTZC_MPCBB_ALL_NPRIV;
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Required GPIO configured non secure */
  GPIOA_S->SECCFGR = ~GPIOA_MASK_SECCFG;
  GPIOB_S->SECCFGR = ~GPIOB_MASK_SECCFG;
  GPIOC_S->SECCFGR = ~GPIOC_MASK_SECCFG;
  GPIOD_S->SECCFGR = ~GPIOD_MASK_SECCFG;

  /* disable MPU */
   MPU->CTRL = 0;
   MPU_NS->CTRL = 0;

  /* Required peripherals configured non secure (default value) / privileged */
  GTZC_TZSC1_S->PRIVCFGR1 = ~TZSC_MASK_R1;
  GTZC_TZSC1_S->PRIVCFGR2 = ~TZSC_MASK_R2;
  GTZC_TZSC1_S->PRIVCFGR3 = ~TZSC_MASK_R3;

  GTZC_TZSC1_S->SECCFGR1 = ~TZSC_MASK_R1;
  GTZC_TZSC1_S->SECCFGR2 = ~TZSC_MASK_R2;
  GTZC_TZSC1_S->SECCFGR3 = ~TZSC_MASK_R3;

  for (i = 0U; i < ARRAY_SIZE(sau_load_cfg); i++)
  {
    SAU->RNR = sau_load_cfg[i].RNR;
    SAU->RBAR = sau_load_cfg[i].RBAR & SAU_RBAR_BADDR_Msk;
    SAU->RLAR = (sau_load_cfg[i].RLAR & SAU_RLAR_LADDR_Msk) |
                SAU_RLAR_ENABLE_Msk;
  }

  secure_internal_flash(0x00, S_IMAGE_SECONDARY_PARTITION_OFFSET-1);

  /* Force memory writes before continuing */
  __DSB();
  /* Flush and refill pipeline with updated permissions */
  __ISB();
  /* Enable SAU */
  TZ_SAU_Enable();

   /* Enable HardFault/busFault and NMI exception in ns.
   * It is up to BL to drive non-secure faults
   * Do not enter in secure on non-secure fault
   */
  SCB->AIRCR  = (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
                           (SCB->AIRCR & 0x0000FFFFU) |
                           SCB_AIRCR_BFHFNMINS_Msk);

  NVIC->ITNS[0U] = RSS_NVIC_INIT_ITNS0_VAL;
  NVIC->ITNS[1U] = RSS_NVIC_INIT_ITNS1_VAL;
  NVIC->ITNS[2U] = RSS_NVIC_INIT_ITNS2_VAL;
  NVIC->ITNS[3U] = RSS_NVIC_INIT_ITNS3_VAL;

  /* Stop systick before jumping */
  HAL_SuspendTick();

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

/**
  * @brief  Perform Jump to the BootLoader
  * @retval None.
  */
static void LOADER_Run(void)
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

  SECURE_loader_run();
}

/**
  * @}
  */
/** @defgroup  FW_UPDATE_Exported_Functions Exported Functions
  * @{
  */

/** @defgroup  FW_UPDATE_Control_Functions Control Functions
  * @{
   */
void FW_UPDATE_Run(void)
{
  uint8_t key = 0U;
  uint8_t exit = 0U;

  /* Print Firmware Update welcome message */
  FW_UPDATE_PrintWelcome();

  while (exit == 0U)
  {
    key = 0U;

    /* Clean the input path */
    COM_Flush();

    /* Receive key */
    if (COM_Receive(&key, 1U, RX_TIMEOUT) == HAL_OK)
    {
      switch (key)
      {
			case 'b' :
					LOADER_Run();
          break;				
      case '1' :
          printf("  -- Install image : reboot\r\n\n");
          NVIC_SystemReset();
          break;
      case '2' :
          FW_UPDATE_SECURE_APP_IMAGE();
          break;
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
      case '3' :
          FW_UPDATE_NONSECURE_APP_IMAGE();
          break;
#endif /*  (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
      case '4' :
          FW_UPDATE_SECURE_DATA_IMAGE();
          break;
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
#if (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
      case '5' :
          FW_UPDATE_NONSECURE_DATA_IMAGE();
          break;
#endif /* (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1) */
      case 'x':
          printf("Exit from FW update menu\r\n");
          exit =1;
          break;
      default:
          printf("Invalid Number !\r");
          break;
      }
      /* Print Main Menu message */
      FW_UPDATE_PrintWelcome();
    }
  }
}

/**
  * @brief  Run FW Update process.
  * @param  None
  * @retval HAL Status.
  */
static HAL_StatusTypeDef FW_UPDATE_SECURE_APP_IMAGE(void)
{
  HAL_StatusTypeDef ret = HAL_ERROR;
  SFU_FwImageFlashTypeDef fw_image_dwl_area;
  ARM_FLASH_INFO *data = LOADER_FLASH_DEV_NAME.GetInfo();
  /* Print Firmware Update welcome message */
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
  printf("Download Secure App Image\r\n");
#else
  printf("Download App Image\r\n");
#endif /* (MCUBOOT_APP_IMAGE_NUMBER == 2) */
  /* Get Info about the download area */
#if  defined(MCUBOOT_PRIMARY_ONLY)
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_0_OFFSET;
  fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_0_SIZE;
#else
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_2_OFFSET;
  fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_2_SIZE;
#endif /* MCUBOOT_PRIMARY_ONLY */
  fw_image_dwl_area.ImageOffsetInBytes = 0x0;
  m_uFlashSectorSize = data->sector_size;
  m_uFlashMinWriteSize = data->program_unit;
  /* Download new firmware image*/
  ret = FW_UPDATE_DownloadNewFirmware(&fw_image_dwl_area);

  if (HAL_OK == ret)
  {
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
    printf("  -- Secure App Image correctly downloaded \r\n\n");
#else
    printf("  -- App Image correctly downloaded \r\n\n");
#endif /* (MCUBOOT_APP_IMAGE_NUMBER == 2) */
    HAL_Delay(1000U);
  }

  return ret;
}
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
/**
  * @brief  Run FW Update process.
  * @param  None
  * @retval HAL Status.
  */
static HAL_StatusTypeDef FW_UPDATE_NONSECURE_APP_IMAGE(void)
{
  HAL_StatusTypeDef ret = HAL_ERROR;
  SFU_FwImageFlashTypeDef fw_image_dwl_area;
  ARM_FLASH_INFO *data = LOADER_FLASH_DEV_NAME.GetInfo();
  /* Print Firmware Update welcome message */
  printf("Download NonSecure App Image\r\n");

  /* Get Info about the download area */
#if  defined(MCUBOOT_PRIMARY_ONLY)
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_1_OFFSET;
#else
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_3_OFFSET;
#endif /* MCUBOOT_PRIMARY_ONLY */
  fw_image_dwl_area.MaxSizeInBytes = FLASH_NS_PARTITION_SIZE;
  fw_image_dwl_area.ImageOffsetInBytes = 0x0;
  m_uFlashSectorSize = data->sector_size;
  m_uFlashMinWriteSize = data->program_unit;
  /* Download new firmware image*/
  ret = FW_UPDATE_DownloadNewFirmware(&fw_image_dwl_area);

  if (HAL_OK == ret)
  {
    printf("  -- NonSecure App Image correctly downloaded \r\n\n");
    HAL_Delay(1000U);
  }

  return ret;
}
#endif /* (MCUBOOT_APP_IMAGE_NUMBER == 2) */

#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
/**
  * @brief  Run FW Update process.
  * @param  None
  * @retval HAL Status.
  */
static HAL_StatusTypeDef FW_UPDATE_SECURE_DATA_IMAGE(void)
{
  HAL_StatusTypeDef ret = HAL_ERROR;
  SFU_FwImageFlashTypeDef fw_image_dwl_area;
  ARM_FLASH_INFO *data = LOADER_FLASH_DEV_NAME.GetInfo();
  /* Print Firmware Update welcome message */
  printf("Download Secure Data Image\r\n");
  /* Get Info about the download area */
#if  defined(MCUBOOT_PRIMARY_ONLY)
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_4_OFFSET;
  fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_4_SIZE;
#else
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_6_OFFSET;
  fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_6_SIZE;
#endif /* MCUBOOT_PRIMARY_ONLY */
  fw_image_dwl_area.ImageOffsetInBytes = 0x0;
  m_uFlashSectorSize = data->sector_size;
  m_uFlashMinWriteSize = data->program_unit;
  /* Download new firmware image*/
  ret = FW_UPDATE_DownloadNewFirmware(&fw_image_dwl_area);

  if (HAL_OK == ret)
  {
    printf("  -- Secure Data Image correctly downloaded \r\n\n");
    HAL_Delay(1000U);
  }

  return ret;
}
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */

#if (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
/**
  * @brief  Run FW Update process.
  * @param  None
  * @retval HAL Status.
  */
static HAL_StatusTypeDef FW_UPDATE_NONSECURE_DATA_IMAGE(void)
{
  HAL_StatusTypeDef ret = HAL_ERROR;
  SFU_FwImageFlashTypeDef fw_image_dwl_area;
  ARM_FLASH_INFO *data = LOADER_FLASH_DEV_NAME.GetInfo();
  /* Print Firmware Update welcome message */
  printf("Download NonSecure Data Image\r\n");
  /* Get Info about the download area */
#if  defined(MCUBOOT_PRIMARY_ONLY)
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_5_OFFSET;
  fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_5_SIZE;
#else
  fw_image_dwl_area.DownloadAddr =  FLASH_AREA_7_OFFSET;
  fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_7_SIZE;
#endif /* defined(MCUBOOT_PRIMARY_ONLY) */
  fw_image_dwl_area.ImageOffsetInBytes = 0x0;
  m_uFlashSectorSize = data->sector_size;
  m_uFlashMinWriteSize = data->program_unit;
  /* Download new firmware image*/
  ret = FW_UPDATE_DownloadNewFirmware(&fw_image_dwl_area);

  if (HAL_OK == ret)
  {
    printf("  -- NonSecure Data Image correctly downloaded \r\n\n");
    HAL_Delay(1000U);
  }

  return ret;
}

#endif /* (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1) */

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup  FW_UPDATE_Private_Functions
  * @{
  */

/**
  * @brief  Display the FW_UPDATE Main Menu choices on HyperTerminal
  * @param  None.
  * @retval None.
  */
static void FW_UPDATE_PrintWelcome(void)
{
  printf("\r\n================ New Fw Image ============================\r\n\n");
  printf("  Reset to trigger Installation ------------------------- 1\r\n\n");
#if (MCUBOOT_APP_IMAGE_NUMBER == 2)
  printf("  Download Secure App Image ----------------------------- 2\r\n\n");
  printf("  Download NonSecure App Image -------------------------- 3\r\n\n");
#else
  printf("  Download App Image ------------------------------------ 2\r\n\n");
#endif /* (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
  printf("  Download Secure Data Image ---------------------------- 4\r\n\n");
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
#if (MCUBOOT_NS_DATA_IMAGE_NUMBER == 1)
  printf("  Download NonSecure Data Image ------------------------- 5\r\n\n");
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */
  printf("  Exit from FW update menu ------------------------------ x\r\n\n");
}
/**
  * @brief Download a new Firmware from the host.
  * @retval HAL status
  */
static HAL_StatusTypeDef FW_UPDATE_DownloadNewFirmware(SFU_FwImageFlashTypeDef *pFwImageDwlArea)
{
  HAL_StatusTypeDef ret = HAL_ERROR;
  COM_StatusTypeDef e_result;
  int32_t ret_arm;
  uint32_t u_fw_size = pFwImageDwlArea->MaxSizeInBytes ;
  uint32_t sector_address;

  /* Clear download area */
  printf("  -- Erasing download area \r\n\n");

  for (sector_address = pFwImageDwlArea->DownloadAddr;
       sector_address < pFwImageDwlArea->DownloadAddr + pFwImageDwlArea->MaxSizeInBytes;
       sector_address += m_uFlashSectorSize)
  {
    ret_arm = LOADER_FLASH_DEV_NAME.EraseSector(sector_address);
    if (ret_arm < 0)
    {
      return HAL_ERROR;
    }
  }

  printf("  -- Send Firmware \r\n\n");

  /* Download binary */
  printf("  -- -- File> Transfer> YMODEM> Send \t\n");

  /*Init of Ymodem*/
  Ymodem_Init();

  /*Receive through Ymodem*/
  e_result = Ymodem_Receive(&u_fw_size, pFwImageDwlArea->DownloadAddr);
  printf("\r\n\n");

  if ((e_result == COM_OK))
  {
    printf("  -- -- Programming Completed Successfully!\r\n\n");
#if defined(__ARMCC_VERSION)
    printf("  -- -- Bytes: %u\r\n\n", u_fw_size);
#else
    printf("  -- -- Bytes: %lu\r\n\n", u_fw_size);
#endif /*  __ARMCC_VERSION */
    ret = HAL_OK;
#if   !defined(MCUBOOT_PRIMARY_ONLY)
    if (u_fw_size <= (pFwImageDwlArea->MaxSizeInBytes - sizeof(MagicTrailerValue)))
    {
      uint32_t MagicAddress =
        pFwImageDwlArea->DownloadAddr + (pFwImageDwlArea->MaxSizeInBytes - sizeof(MagicTrailerValue));
      /* write the magic to trigger installation at next reset */
#if defined(__ARMCC_VERSION)
      printf("  Write Magic Trailer at %x\r\n\n", MagicAddress);
#else
      printf("  Write Magic Trailer at %lx\r\n\n", MagicAddress);
#endif /*  __ARMCC_VERSION */
      if (LOADER_FLASH_DEV_NAME.ProgramData(MagicAddress, MagicTrailerValue, sizeof(MagicTrailerValue)) != ARM_DRIVER_OK)
      {
        ret = HAL_ERROR;
      }
    }
#endif /* !defined(MCUBOOT_PRIMARY_ONLY) */
  }
  else if (e_result == COM_ABORT)
  {
    printf("  -- -- !!Aborted by user!!\r\n\n");
    COM_Flush();
    ret = HAL_ERROR;
  }
  else
  {
    printf("  -- -- !!Error during file download!!\r\n\n");
    ret = HAL_ERROR;
    HAL_Delay(500U);
    COM_Flush();
  }

  return ret;
}


/**
  * @}
  */

/** @defgroup FW_UPDATE_Callback_Functions Callback Functions
  * @{
  */

/**
  * @brief  Ymodem Header Packet Transfer completed callback.
  * @param  uFileSize Dimension of the file that will be received (Bytes).
  * @retval None
  */
HAL_StatusTypeDef Ymodem_HeaderPktRxCpltCallback(uint32_t uFlashDestination, uint32_t uFileSize)
{
  /*Reset of the ymodem variables */
  m_uFileSizeYmodem = 0U;
  m_uPacketsReceived = 0U;
  m_uNbrBlocksYmodem = 0U;

  /*Filesize information is stored*/
  m_uFileSizeYmodem = uFileSize;

  /* compute the number of 1K blocks */
  m_uNbrBlocksYmodem = (m_uFileSizeYmodem + (PACKET_1K_SIZE - 1U)) / PACKET_1K_SIZE;

  /* NOTE : delay inserted for Ymodem protocol*/
  HAL_Delay(1000);
  return HAL_OK;
}

extern uint32_t total_size_received;
/**
  * @brief  Ymodem Data Packet Transfer completed callback.
  * @param  pData Pointer to the buffer.
  * @param  uSize Packet dimension (Bytes).
  * @retval None
  */
HAL_StatusTypeDef Ymodem_DataPktRxCpltCallback(uint8_t *pData, uint32_t uFlashDestination, uint32_t uSize)
{
  int32_t ret;
  m_uPacketsReceived++;

  /*Increase the number of received packets*/
  if (m_uPacketsReceived == m_uNbrBlocksYmodem) /*Last Packet*/
  {
    /*Extracting actual payload from last packet*/
    if (0 == (m_uFileSizeYmodem % PACKET_1K_SIZE))
    {
      /* The last packet must be fully considered */
      uSize = PACKET_1K_SIZE;
    }
    else
    {
      /* The last packet is not full, drop the extra bytes */
      uSize = m_uFileSizeYmodem - ((uint32_t)(m_uFileSizeYmodem / PACKET_1K_SIZE) * PACKET_1K_SIZE);
    }

    m_uPacketsReceived = 0U;
  }
  /*Adjust dimension to 64-bit length */
  if (uSize %  m_uFlashMinWriteSize != 0U)
  {
    memset(&pData[uSize], 0xff, (m_uFlashMinWriteSize - (uSize %  m_uFlashMinWriteSize)));
    uSize += (m_uFlashMinWriteSize - (uSize %  m_uFlashMinWriteSize));
  }
  /* Write Data in Flash - size has to be 64-bit aligned */
  ret = LOADER_FLASH_DEV_NAME.ProgramData(uFlashDestination, pData, uSize);
  if (ret != ARM_DRIVER_OK)
  {
    /*Reset of the ymodem variables */
    m_uFileSizeYmodem = 0U;
    m_uPacketsReceived = 0U;
    m_uNbrBlocksYmodem = 0U;
    return HAL_ERROR;
  }
  else
    return HAL_OK;
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
