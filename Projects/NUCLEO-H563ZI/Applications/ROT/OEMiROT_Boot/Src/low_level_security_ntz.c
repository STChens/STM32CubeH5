/**
  ******************************************************************************
  * @file    low_level_security.c
  * @author  MCD Application Team
  * @brief   security protection implementation for secure boot on STM32H5xx
  *
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
#include <string.h>
#include "boot_hal_cfg.h"
#include "boot_hal_flowcontrol.h"
#include "mpu_armv8m_drv.h"
#include "region_defs.h"
#include "mcuboot_config/mcuboot_config.h"
#include "low_level_security.h"
#ifdef OEMIROT_DEV_MODE
#define BOOT_LOG_LEVEL BOOT_LOG_LEVEL_INFO
#else
#define BOOT_LOG_LEVEL BOOT_LOG_LEVEL_OFF
#endif /* OEMIROT_DEV_MODE  */
#include "bootutil/bootutil_log.h"
#include "low_level_rng.h"
#include "target_cfg.h"
#include "bootutil_priv.h"

/** @defgroup OEMIROT_SECURITY_Private_Defines  Private Defines
  * @{
  */
/* DUAL BANK page size */
#define PAGE_SIZE FLASH_AREA_IMAGE_SECTOR_SIZE

#define PAGE_MAX_NUMBER_IN_BANK 0x7F

/* OEMIROT_Boot Vector Address  */
#define OEMIROT_BOOT_VTOR_ADDR ((uint32_t)(BL2_CODE_START))

/**************************
  * Initial configuration *
  *************************/

/* MPU configuration
  ================== */
const struct mpu_armv8m_region_cfg_t mpu_region_cfg_init[] = {
  /* Region 0: Allows execution of BL2 */
  {
    0,
    FLASH_BASE + FLASH_AREA_BL2_OFFSET,
    FLASH_BASE + FLASH_AREA_BL2_OFFSET + FLASH_AREA_BL2_SIZE - 1,
    MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
    MPU_ARMV8M_XN_EXEC_OK,
    MPU_ARMV8M_AP_RO_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R0,
    FLOW_CTRL_MPU_I_EN_R0,
    FLOW_STEP_MPU_I_CH_R0,
    FLOW_CTRL_MPU_I_CH_R0,
#endif /* FLOW_CONTROL */
  },
  /* Region 1: Allows RW access for flash area outside of BOOT code */
  {
    1,
    FLASH_BASE + FLASH_AREA_BL2_OFFSET + FLASH_AREA_BL2_SIZE,
    FLASH_BASE + FLASH_AREA_END_OFFSET - 1,
    MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RW_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R1,
    FLOW_CTRL_MPU_I_EN_R1,
    FLOW_STEP_MPU_I_CH_R1,
    FLOW_CTRL_MPU_I_CH_R1,
#endif /* FLOW_CONTROL */
  },
  /* Region 2: Allows RW access to BL2 SRAM */
  {
    2,
    BL2_SRAM_AREA_BASE,
    BL2_SRAM_AREA_END,
    MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RW_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R2,
    FLOW_CTRL_MPU_I_EN_R2,
    FLOW_STEP_MPU_I_CH_R2,
    FLOW_CTRL_MPU_I_CH_R2,
#endif /* FLOW_CONTROL */
  },
  /* Region 4: Allows RW access to peripherals */
  {
    4,
    PERIPH_BASE,
    PERIPH_BASE + 0xFFFFFFF,
    MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RW_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R4,
    FLOW_CTRL_MPU_I_EN_R4,
    FLOW_STEP_MPU_I_CH_R4,
    FLOW_CTRL_MPU_I_CH_R4,
#endif /* FLOW_CONTROL */
  },
  /* Region 5: Allows read access to STM32 descriptors */
  /* start = the start of the new descriptor address and the
     end = the end of the old descriptor address */
  {
    5,
    STM32_DESCRIPTOR_BASE_NS,
    STM32_DESCRIPTOR_END_NS + NSS_LIB_SIZE,
    MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RO_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R5,
    FLOW_CTRL_MPU_I_EN_R5,
    FLOW_STEP_MPU_I_CH_R5,
    FLOW_CTRL_MPU_I_CH_R5,
#endif /* FLOW_CONTROL */
  },
  /* Region 6: Allows read access to Engi bytes */
  {
    6,
    ENGI_BASE_NS,
    ENGI_BASE_NS - 1, /* + ENGI_SIZE */
    MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RO_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R6,
    FLOW_CTRL_MPU_I_EN_R6,
    FLOW_STEP_MPU_I_CH_R6,
    FLOW_CTRL_MPU_I_CH_R6,
#endif /* FLOW_CONTROL */
  },
  /* Region 7: Allows RW access to OBKeys HDPL1&2&3 area */
  {
    7,
    FLASH_OBK_BASE_S + OBK_HDPL1_OFFSET,
    FLASH_OBK_BASE_S + OBK_HDPL3_END,
    MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RW_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_I_EN_R7,
    FLOW_CTRL_MPU_I_EN_R7,
    FLOW_STEP_MPU_I_CH_R7,
    FLOW_CTRL_MPU_I_CH_R7,
#endif /* FLOW_CONTROL */
  },
};

const struct mpu_armv8m_region_cfg_t mpu_region_cfg_appli[] = {
  /* First region in this list is configured only at this stage, */
  /* the region will be activated later by RSS jump service. Following regions */
  /*  in this list are configured and activated at this stage. */

  /* Region 1 (overwrite): Allows execution of appli */
  {
    1,
    FLASH_BASE + APP_IMAGE_PRIMARY_PARTITION_OFFSET,
    FLASH_BASE + APP_IMAGE_PRIMARY_PARTITION_OFFSET + FLASH_APP_PARTITION_SIZE - 1 - (~MPU_RLAR_LIMIT_Msk +1),
    MPU_ARMV8M_MAIR_ATTR_DATA_IDX,
    MPU_ARMV8M_XN_EXEC_OK,
    MPU_ARMV8M_AP_RO_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_A_EN_R1,
    FLOW_CTRL_MPU_A_EN_R1,
    FLOW_STEP_MPU_A_CH_R1,
    FLOW_CTRL_MPU_A_CH_R1,
#endif /* FLOW_CONTROL */
  },
  /* Region 3: Allows RW access to end of flash area for image confirmation (swap mode) */
  {
    3,
    FLASH_BASE + APP_IMAGE_PRIMARY_PARTITION_OFFSET + FLASH_APP_PARTITION_SIZE  - (~MPU_RLAR_LIMIT_Msk +1),
    FLASH_BASE + FLASH_AREA_END_OFFSET - 1,
    MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX,
    MPU_ARMV8M_XN_EXEC_NEVER,
    MPU_ARMV8M_AP_RW_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_A_EN_R2,
    FLOW_CTRL_MPU_A_EN_R2,
    FLOW_STEP_MPU_A_CH_R2,
    FLOW_CTRL_MPU_A_CH_R2,
#endif /* FLOW_CONTROL */
  },  
  /* Region 5 (Overwrite): Allow NSSLIB and system bootloader to execute  */
  {
    5,
    0x0BF90000,
    STM32_DESCRIPTOR_END_NS + NSS_LIB_SIZE,
    MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
    MPU_ARMV8M_XN_EXEC_OK,
    MPU_ARMV8M_AP_RO_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_A_EN_R3,
    FLOW_CTRL_MPU_A_EN_R3,
    FLOW_STEP_MPU_A_CH_R3,
    FLOW_CTRL_MPU_A_CH_R3,
#endif /* FLOW_CONTROL */
  },  
};

/* Product state control
   ===================== */
static const uint32_t ProductStatePrioList[] = {
        OB_PROD_STATE_OPEN,
        OB_PROD_STATE_PROVISIONING,
        OB_PROD_STATE_IROT_PROVISIONED,
        OB_PROD_STATE_TZ_CLOSED,
        OB_PROD_STATE_CLOSED,
        OB_PROD_STATE_LOCKED
};

#define NB_PRODUCT_STATE (sizeof(ProductStatePrioList) / sizeof(uint32_t))

#if defined(MCUBOOT_EXT_LOADER)
/*********************************
 * Loader specific configuration *
 *********************************/
/* GPIO configuration
  =================== */
/*----------------------|  USART  |-------------------------------------*/
/* USART: USART1 + USART2 + USART 3*/
/* USART1 (RX = PA10, TX = PA9), USART2 (RX = PA3, TX = PA2)  */
/* USART3 (RX = PD9, TX = PD8) */

/*----------------------|  SPI  |-------------------------------------*/
/* SPI: SPI1 + SPI2 + SPI3*/
/* SPI1: _MOSI  =  PA7, _MISO = PA6 , _SCK = PA5, _NSS= PA4 */
/* SPI2: _MOSI  =  PC1, _MISO = PB14 , _SCK = PB10, _NSS= PB12 */
/* SPI3: _MOSI  =  PC12, _MISO = PC11 , _SCK = PC10, !! _NSS= PA15 */

/*----------------------| I2C  |---------------------------------------*/
/*I2C:  I2C3 + I2C4*/
/*I2C3 (_SCL: PA8, _SDA:PC9 ) */
/*I2C4 (_SCL=PD12, _SDA=PD13 ) */

/*----------------------| I3C  |---------------------------------------*/
/*I3C1 (_SCL: PB6, _SDA:PB7 ) */

/*----------------------|  USB |---------------------------------------*/
/* USB*/
/*USB (_DM = PA11, _DP = PA12 )*/

/*----------------------|  FDCAN  |------------------------------------*/
/*FDCAN2 */
/*CAN (_RX = PB5, _TX = PB13 ) */

#define GPIOA_MASK_SECCFG    (GPIO_PIN_10 | GPIO_PIN_9  | GPIO_PIN_3  | GPIO_PIN_2  | GPIO_PIN_7  | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4 | \
                              GPIO_PIN_15 | GPIO_PIN_8  | GPIO_PIN_11 | GPIO_PIN_12)
#define GPIOB_MASK_SECCFG    (GPIO_PIN_14 |\
                              GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_6  | GPIO_PIN_7  | GPIO_PIN_5 | GPIO_PIN_13)
#define GPIOC_MASK_SECCFG    (GPIO_PIN_1  | GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_9)
#define GPIOD_MASK_SECCFG    (GPIO_PIN_9  | GPIO_PIN_8  | GPIO_PIN_12 | GPIO_PIN_13)

/* Peripherals configuration
   ========================= */
/*----------------------|  USART  |-------------------------------------*/
/* USART: USART1 + USART2 + USART3 */

/*----------------------|  SPI  |-------------------------------------*/
/* SPI: SPI1 + SPI2 + SPI3 */

/*----------------------| I2C  |---------------------------------------*/
/* I2C: I2C3 + I2C4 */

/*----------------------| I3C  |---------------------------------------*/
/* I3C: I3C1*/

/*----------------------|  USB |---------------------------------------*/
/* USB */

/*----------------------|  FDCAN  |------------------------------------*/
/* FDCAN2 */
/* Due to HW constraint, FDCAN1 and FDCAN2 shall be set as non-secure in order to grant FDCAN2 to non-secure (Bootloader). */

/*----------------------|  ICACHE  |------------------------------------*/
/* ICACHE */

/*----------------------|  IWDG |------------------------------------*/
/* IWDG */


/* NVIC configuration
   ================== */
/*Interrupts 0 .. 31 */
/*in ITNS0 GPDMA1_Channel0_IRQn | GPDMA1_Channel1_IRQn | GPDMA1_Channel2_IRQn bit27|28|29  is non secure (1) */
#define RSS_NVIC_INIT_ITNS0_VAL      (0x38000000U)

/*Interrupts 32 .. 63 */
/*in ITNS1 no bit is non secure (1) */
#define RSS_NVIC_INIT_ITNS1_VAL      (0x00000000U)

/*Interrupts 64 .. 95 */
/*in ITNS2 OTG_FS_IRQn 74, i.e bit 10 is non secure (1) */
#define RSS_NVIC_INIT_ITNS2_VAL      (0x00000400U)

/*Interrupts 96 .. 128 */
/*I3C1_EV_IRQn = 123*/
#define RSS_NVIC_INIT_ITNS3_VAL      (0x08000000U)

/* SRAM1 configuration
   =================== */
/* SRAM1 NB super-block */
#define GTZC_MPCBB1_NB_VCTR (16U)

/* SRAM3 NB super-block */
#define GTZC_MPCBB3_NB_VCTR (20U)

/* MPCBB : All SRAM block non secure */
#define GTZC_MPCBB_ALL_NSEC (0x00000000UL)

/* MPCBB : All SRAM block non privileged + privileged */
#define GTZC_MPCBB_ALL_NPRIV (0x00000000UL)

#ifdef OEMIROT_MPU_PROTECTION

#if 0 // FIXME we disable MPU to execute loader from system flash
/* MPU configuration
   ================= */
static const struct mpu_armv8m_region_cfg_t region_cfg_loader_s[] =
{
  /* Region 5 (Overwrite): Allow NSSLIB and system bootloader to execute  */
  {
    5,
    0x0BF90000,
    BOOTLOADER_BASE_NS + BOOTLOADER_SIZE - 1,
    MPU_ARMV8M_MAIR_ATTR_CODE_IDX,
    MPU_ARMV8M_XN_EXEC_OK,
    MPU_ARMV8M_AP_RO_PRIV_ONLY,
    MPU_ARMV8M_SH_NONE,  
#ifdef FLOW_CONTROL
    FLOW_STEP_MPU_L_EN_R7,
    FLOW_CTRL_MPU_L_EN_R7,
    FLOW_STEP_MPU_L_CH_R7,
    FLOW_CTRL_MPU_L_CH_R7,
#endif /* FLOW_CONTROL */
  },
};
#endif /* FIXME we disable MPU to execute loader from system flash */
#endif
#endif /* MCUBOOT_EXT_LOADER */

/**
  * @}
  */
/* Private function prototypes -----------------------------------------------*/
/** @defgroup OEMIROT_SECURITY_Private_Functions  Private Functions
  * @{
  */
static void gtzc_init_cfg(void);
static void mpu_init_cfg(void);
static void mpu_appli_cfg(void);
static void flash_priv_cfg(void);
static void ram_init_cfg(void);
#if defined(MCUBOOT_EXT_LOADER)
static void hdpext_loader_cfg(void);
#endif
#if defined(MCUBOOT_EXT_LOADER)
static void mpu_loader_cfg(void);
#if defined(MCUBOOT_PRIMARY_ONLY)
static void secure_internal_flash(uint32_t offset_start, uint32_t offset_end);
#endif /* MCUBOOT_PRIMARY_ONLY */
#endif /* MCUBOOT_EXT_LOADER */
static void active_tamper(void);
/**
  * @}
  */

/** @defgroup OEMIROT_SECURITY_Exported_Functions Exported Functions
  * @{
  */
#if defined(MCUBOOT_EXT_LOADER)
/**
  * @brief  Update the runtime security protections for application start
  *
  * @param  None
  * @retval None
  */
void LL_SECU_UpdateLoaderRunTimeProtections(void)
{
  /* Set MPU to enable execution of secure /non secure  loader */
  mpu_loader_cfg();

  /* extend HDPL2 HDP in user flash except DWL area */
  hdpext_loader_cfg();

}
#endif /* MCUBOOT_EXT_LOADER */
/**
  * @brief  Apply the runtime security  protections to
  *
  * @param  None
  * @note   By default, the best security protections are applied
  * @retval None
  */
void LL_SECU_ApplyRunTimeProtections(void)
{
  /* Configure NonSecure memory */
  gtzc_init_cfg();

  /* Set MPU to forbid execution outside of immutable code  */
  mpu_init_cfg();

  /* With OEMIROT_DEV_MODE , active tamper calls Error_Handler */
  /* Error_Handler requires sau_init_cfg */
  active_tamper();

  /* Configure Flash Privilege access */
  flash_priv_cfg();

  /* Configure SRAM ECC */
  ram_init_cfg();
}

/**
  * @brief  Update the runtime security protections for application start
  *
  * @param  None
  * @retval None
  */
void LL_SECU_UpdateRunTimeProtections(void)
{
  /* Update MPU config for application execeution */
  mpu_appli_cfg();
}

/**
  * @brief  Check if the Static security protections are configured.
  *         Those protections are not impacted by a Reset. They are set using the Option Bytes.
  *         When the device is locked, these protections cannot be changed anymore.
  * @param  None
  * @note   By default, the best security protections are applied to the different
  *         flash sections in order to maximize the security level for the specific MCU.
  * @retval None
  */
void LL_SECU_CheckStaticProtections(void)
{
  static FLASH_OBProgramInitTypeDef flash_option_bytes_bank1 = {0};
  static FLASH_OBProgramInitTypeDef flash_option_bytes_bank2 = {0};
  uint32_t start;
  uint32_t end;
  uint32_t i;


  /* Get bank1 OB  */
  flash_option_bytes_bank1.Banks = FLASH_BANK_1;
  flash_option_bytes_bank1.BootConfig = OB_BOOT_NS;
  HAL_FLASHEx_OBGetConfig(&flash_option_bytes_bank1);

  /* Get bank2 OB  */
  flash_option_bytes_bank2.Banks = FLASH_BANK_2;
  HAL_FLASHEx_OBGetConfig(&flash_option_bytes_bank2);

  /* Check TZEN = 1 , we are in secure */
  if ((flash_option_bytes_bank1.USERConfig2 & FLASH_OPTSR2_TZEN) != OB_TZEN_DISABLE)
  {
    BOOT_LOG_ERR("Unexpected value for TZEN");
    Error_Handler();
  }


  /* Check if swap bank is reset */
  if ((flash_option_bytes_bank1.USERConfig & FLASH_OPTSR_SWAP_BANK) != 0)
  {
    BOOT_LOG_ERR("Unexpected value for swap bank configuration");
    Error_Handler();
  }

  /* Check BOOT UBE */
  if ((flash_option_bytes_bank1.USERConfig & FLASH_OPTSR_BOOT_UBE) != OB_UBE_OEM_IROT)
  {
    BOOT_LOG_ERR("Unexpected value for BOOT UBE configuration");
    Error_Handler();
  }

  /* Check secure boot address */
  if (flash_option_bytes_bank1.BootAddr != BL2_BOOT_VTOR_ADDR)
  {
    BOOT_LOG_INF("BootAddr 0x%x", (int)flash_option_bytes_bank1.BootAddr);
    BOOT_LOG_ERR("Unexpected value for SEC BOOT Address");
    Error_Handler();
  }

#ifdef  OEMIROT_WRP_PROTECT_ENABLE
  uint32_t val;
  /* Check flash write protection */
  start = FLASH_AREA_BL2_OFFSET / PAGE_SIZE;
  end = (FLASH_AREA_BL2_OFFSET + FLASH_AREA_BL2_SIZE - 1) / PAGE_SIZE;
  val = 0;
  for (i = (start/4); i <= (end/4); i++)
  {
    val |= (1 << i);
  }
  if ((flash_option_bytes_bank1.WRPState != OB_WRPSTATE_ENABLE)
      || (flash_option_bytes_bank1.WRPSector != val))
  {
    BOOT_LOG_INF("BANK 1 flash write protection group 0x%x: OB 0x%x",
                 (int)val, (int)flash_option_bytes_bank1.WRPSector);
    BOOT_LOG_ERR("Unexpected value for write protection ");
    Error_Handler();
  }

#endif /* OEMIROT_WRP_PROTECT_ENABLE */

#ifdef  OEMIROT_HDP_PROTECT_ENABLE
  /* Check secure user flash protection (HDP) */
  start = 0;
  end = (FLASH_BL2_HDP_END) / PAGE_SIZE;

  if ((start != flash_option_bytes_bank1.HDPStartSector)
    || (end != flash_option_bytes_bank1.HDPEndSector))
  {
    BOOT_LOG_INF("BANK 1 hide protection [%d, %d] : OB [%d, %d]",
                 (int)start,
                 (int)end,
                 (int)flash_option_bytes_bank1.HDPStartSector,
                 (int)flash_option_bytes_bank1.HDPEndSector);
    BOOT_LOG_ERR("Unexpected value for hide protection");
    Error_Handler();
  }


#else /* OEMIROT_HDP_PROTECT_ENABLE */
#endif /* OEMIROT_HDP_PROTECT_ENABLE */

#ifdef OEMIROT_SECURE_USER_SRAM2_ERASE_AT_RESET
  /* Check SRAM2 ERASE on reset */
  if ((flash_option_bytes_bank1.USERConfig2 & FLASH_OPTSR2_SRAM2_RST) != 0)
  {
    BOOT_LOG_ERR("Unexpected value for SRAM2 ERASE at Reset");
    Error_Handler();
  }
#endif /*OEMIROT_SECURE_USER_SRAM2_ERASE_AT_RESET */

#ifdef OEMIROT_SECURE_USER_SRAM2_ECC
  /* Check SRAM2 ECC */
  if ((flash_option_bytes_bank1.USERConfig2 & FLASH_OPTSR2_SRAM2_ECC) != 0)
  {
    BOOT_LOG_ERR("Unexpected value for SRAM2 ECC");
    Error_Handler();
  }
#endif /* OEMIROT_SECURE_USER_SRAM2_ECC */

#ifdef OEMIROT_OB_BOOT_LOCK
  /* Check Boot lock protection */
  if (flash_option_bytes_bank1.BootLock != OEMIROT_OB_BOOT_LOCK)
  {
    BOOT_LOG_INF("BootLock 0x%x", (int)flash_option_bytes_bank1.BootLock);
    BOOT_LOG_ERR("Unexpected value for SEC BOOT LOCK");
    Error_Handler();
  }
#endif /* OEMIROT_OB_BOOT_LOCK */

  /* Check Product State : boot if current Product State is greater or equal to selected Product State */
  /* Identify product state mini selected */
  for (i = 0U; i < NB_PRODUCT_STATE; i++)
  {
    if (ProductStatePrioList[i] == OEMIROT_OB_PRODUCT_STATE_VALUE)
    {
      break;
    }
  }
  if (i >= NB_PRODUCT_STATE)
  {
    Error_Handler();
  }
  /* Control if current product state is allowed */
  for (; i < NB_PRODUCT_STATE; i++)
  {
    if (ProductStatePrioList[i] == flash_option_bytes_bank1.ProductState)
    {
      break;
    }
  }
  if (i >= NB_PRODUCT_STATE)
  {
    Error_Handler();
  }
}

/**
  * @brief  Memory Config Init
  * @param  None
  * @retval None
  */
static void  gtzc_init_cfg(void)
{
  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    __HAL_RCC_GTZC1_CLK_ENABLE();
    /* no FLOW control : this configuration is not part of the security but this is just for functionality */
  }
  /* verification stage */
  else
  {
    /* nothing to do : this configuration is not part of the security but this is just for functionality */
  }
}

#if defined(MCUBOOT_EXT_LOADER)

/**
  * @brief  configure HDP extension before jumping into loader
  * @param  None
  * @retval None
  */
static void hdpext_loader_cfg(void)
{
  FLASH_HDPExtensionTypeDef hdp_extension;
  uint32_t hdp1_start = 0U;
  uint32_t hdp1_end = 0U;
  uint32_t hdp2_start = 0U;
  uint32_t hdp2_end = 0U;
  uint32_t first_allowed = 0U;
  uint32_t hdp_ext = 0U;
  uint32_t hdp1_ext = 0U;
  uint32_t hdp2_ext = 0U;
#if defined(MCUBOOT_PRIMARY_ONLY)
  uint32_t dwl_offset = APP_IMAGE_PRIMARY_PARTITION_OFFSET;
#else
  uint32_t dwl_offset = APP_IMAGE_SECONDARY_PARTITION_OFFSET;
#endif /* MCUBOOT_PRIMARY_ONLY */

  /* Check HDP configuration */
  /* From 0 to 128 (0 HDP not activated) */
  hdp1_start = ((FLASH->HDP1R_CUR & FLASH_HDPR_HDP_STRT) >> FLASH_HDPR_HDP_STRT_Pos) + 1U;
  hdp1_end = ((FLASH->HDP1R_CUR & FLASH_HDPR_HDP_END) >> FLASH_HDPR_HDP_END_Pos) + 1U;
  if (hdp1_end < hdp1_start)
  {
    hdp1_end = 0;
  }
  hdp2_start = ((FLASH->HDP2R_CUR & FLASH_HDPR_HDP_STRT) >> FLASH_HDPR_HDP_STRT_Pos) + 1U;
  hdp2_end = ((FLASH->HDP2R_CUR & FLASH_HDPR_HDP_END) >> FLASH_HDPR_HDP_END_Pos) + 1U;
  if (hdp2_end < hdp2_start)
  {
    hdp2_end = 0;
  }
  first_allowed = dwl_offset / FLASH_SECTOR_SIZE;

  /* Dwl area starts from bank2 */
  /* From 0 to 127 (0 first sector configured as dwl area) */
  if (first_allowed >= FLASH_SECTOR_NB)
  {
    first_allowed -= FLASH_SECTOR_NB;
    hdp1_ext = FLASH_SECTOR_NB - ((hdp1_end == 0U) ? 1U : hdp1_end);
    /* HDP native not configured */
    if (first_allowed > hdp2_end)
    {
      hdp2_ext = first_allowed - ((hdp2_end == 0U) ? 1U : hdp2_end);
    }
    else if ((first_allowed == 0U) && (hdp2_end == 0U))
    {
      hdp2_ext = 0U;
    }
    else
    {
      /* Dwl area under native HDP */
      Error_Handler();
    }
  }
  /* Dwl area starts from bank1 */
  else
  {
    hdp2_ext = 0U;
    if (first_allowed > hdp1_end)
    {
      hdp1_ext = first_allowed - ((hdp1_end == 0U) ? 1U : hdp1_end);
    }
    else if ((first_allowed == 0U) && (hdp1_end == 0U))
    {
      hdp1_ext = 0U;
    }
    else
    {
      /* Dwl area under native HDP */
      Error_Handler();
    }
  }

  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    hdp_extension.Banks = FLASH_BANK_1;
    hdp_extension.NbSectors = hdp1_ext;
    if (HAL_OK == HAL_FLASHEx_ConfigHDPExtension(&hdp_extension))
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_HDPEXT_L_EN_B1, FLOW_CTRL_HDPEXT_L_EN_B1);
    }
    else
    {
      Error_Handler();
    }

    hdp_extension.Banks = FLASH_BANK_2;
    hdp_extension.NbSectors = hdp2_ext;
    if (HAL_OK == HAL_FLASHEx_ConfigHDPExtension(&hdp_extension))
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_HDPEXT_L_EN_B2, FLOW_CTRL_HDPEXT_L_EN_B2);
    }
    else
    {
      Error_Handler();
    }
  }
  /* verification stage */
  else
  {
    hdp_ext = ((FLASH->HDPEXTR & FLASH_HDPEXTR_HDP1_EXT_Msk) >> FLASH_HDPEXTR_HDP1_EXT_Pos);
    if (hdp_ext != hdp1_ext)
    {
      Error_Handler();
    }
    else
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_HDPEXT_L_CH_B1, FLOW_CTRL_HDPEXT_L_CH_B1);
    }

    hdp_ext = ((FLASH->HDPEXTR & FLASH_HDPEXTR_HDP2_EXT_Msk) >> FLASH_HDPEXTR_HDP2_EXT_Pos);
    if (hdp_ext != hdp2_ext)
    {
      Error_Handler();
    }
    else
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_HDPEXT_L_CH_B2, FLOW_CTRL_HDPEXT_L_CH_B2);
    }
  }
}
#endif /* MCUBOOT_EXT_LOADER */


/**
  * @brief  mpu init
  * @param  None
  * @retval None
  */
static void mpu_init_cfg(void)
{
#ifdef OEMIROT_MPU_PROTECTION
  struct mpu_armv8m_dev_t dev_mpu_s = { MPU_BASE };
  int32_t i;

  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    /* configure secure MPU regions */
    for (i = 0; i < ARRAY_SIZE(mpu_region_cfg_init); i++)
    {
      if (mpu_armv8m_region_enable(&dev_mpu_s,
        (struct mpu_armv8m_region_cfg_t *)&mpu_region_cfg_init[i]) != MPU_ARMV8M_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, mpu_region_cfg_init[i].flow_step_enable,
                                             mpu_region_cfg_init[i].flow_ctrl_enable);
      }
    }

    /* enable secure MPU */
    mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_DISABLE, HARDFAULT_NMI_ENABLE);
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_MPU_I_EN, FLOW_CTRL_MPU_I_EN);
  }
  /* verification stage */
  else
  {
    /* check secure MPU regions */
    for (i = 0; i < ARRAY_SIZE(mpu_region_cfg_init); i++)
    {
      if (mpu_armv8m_region_enable_check(&dev_mpu_s,
        (struct mpu_armv8m_region_cfg_t *)&mpu_region_cfg_init[i]) != MPU_ARMV8M_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, mpu_region_cfg_init[i].flow_step_check,
                                             mpu_region_cfg_init[i].flow_ctrl_check);
      }
    }

    /* check secure MPU */
    if (mpu_armv8m_check(&dev_mpu_s, PRIVILEGED_DEFAULT_DISABLE,
                      HARDFAULT_NMI_ENABLE) != MPU_ARMV8M_OK)
    {
      Error_Handler();
    }
    else
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_MPU_I_CH, FLOW_CTRL_MPU_I_CH);
    }
  }
#endif /* OEMIROT_MPU_PROTECTION */
}

/**
  * @brief  configure RAM ECC detection
  * @param  None
  * @retval None
  */
static void ram_init_cfg(void)
{
  RAMCFG_HandleTypeDef hramcfg2;
  hramcfg2.Instance = RAMCFG_SRAM2;

  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    __HAL_RCC_RAMCFG_CLK_ENABLE();

    /* RAMCFG_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(RAMCFG_IRQn, 0x0U, 0x0U);
    HAL_NVIC_EnableIRQ(RAMCFG_IRQn);

    /* Enable IT in case of Double error detection : ECC is activated through option bytes */
    __HAL_RAMCFG_ENABLE_IT(&hramcfg2, RAMCFG_IT_DOUBLEERR);

    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_RAMCFG_I_EN1, FLOW_CTRL_RAMCFG_I_EN1);
  }
  /* verification stage */
  else
  {
    if ((hramcfg2.Instance->IER & RAMCFG_IT_DOUBLEERR) != RAMCFG_IT_DOUBLEERR)
    {
      Error_Handler();
    }
    else
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_RAMCFG_I_CH1, FLOW_CTRL_RAMCFG_I_CH1);
    }
  }
}


static void mpu_appli_cfg(void)
{
#ifdef OEMIROT_MPU_PROTECTION
  static struct mpu_armv8m_dev_t dev_mpu_s = { MPU_BASE };
  int32_t i;
  enum mpu_armv8m_error_t status;

  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    /* configure secure MPU regions */
    for (i = 0; i < ARRAY_SIZE(mpu_region_cfg_appli); i++)
    {
      /* First region configured should be activated by RSS JUMP service,
         execution rights given to primary code slot */
      if ( i == 0 )
      {
        status = mpu_armv8m_region_config_only(&dev_mpu_s, (struct mpu_armv8m_region_cfg_t *)&mpu_region_cfg_appli[i]);
      }
      else
      {
        status = mpu_armv8m_region_enable(&dev_mpu_s, (struct mpu_armv8m_region_cfg_t *)&mpu_region_cfg_appli[i]);
      }

      if (status != MPU_ARMV8M_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, mpu_region_cfg_appli[i].flow_step_enable,
                                             mpu_region_cfg_appli[i].flow_ctrl_enable);
      }
    }
  }
  else
  {
    /* check secure MPU regions */
    for (i = 0; i < ARRAY_SIZE(mpu_region_cfg_appli); i++)
    {
      /* First region configured should be activated by RSS JUMP service,
         execution rights given to primary code slot */
      if (i == 0)
      {
        status = mpu_armv8m_region_config_only_check(&dev_mpu_s, (struct mpu_armv8m_region_cfg_t *)&mpu_region_cfg_appli[i]);
      }
      else
      {
        status = mpu_armv8m_region_enable_check(&dev_mpu_s, (struct mpu_armv8m_region_cfg_t *)&mpu_region_cfg_appli[i]);
      }

      if (status != MPU_ARMV8M_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, mpu_region_cfg_appli[i].flow_step_check,
                                             mpu_region_cfg_appli[i].flow_ctrl_check);
      }
    }
  }
#endif /* OEMIROT_MPU_PROTECTION */
}
#if defined(MCUBOOT_EXT_LOADER)
static void mpu_loader_cfg(void)
{
#ifdef OEMIROT_MPU_PROTECTION
#if 1  
  struct mpu_armv8m_dev_t dev_mpu_s = { MPU_BASE };
  
  /* we just disable MPU before jumping to loader, so region_cfg_loader_s is not used*/
  mpu_armv8m_disable(&dev_mpu_s);
#else
  uint32_t i = 0U;
  /* Secure coding  : volatile variable usage to force compiler to reload SBS->CSLCKR register address */
  __IO uint32_t read_reg = (uint32_t) &SBS->CSLCKR;

  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    /* configure additional secure MPU region */
    for (i = 0U; i < ARRAY_SIZE(region_cfg_loader_s); i++)
    {
      if (mpu_armv8m_region_enable(&dev_mpu_s,
        (struct mpu_armv8m_region_cfg_t *)&region_cfg_loader_s[i]) != MPU_ARMV8M_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, region_cfg_loader_s[i].flow_step_enable,
                          region_cfg_loader_s[i].flow_ctrl_enable);
      }
    }
  }
  /* verification stage */
  else
  {
    /* check secure MPU regions */
    for (i = 0U; i < ARRAY_SIZE(region_cfg_loader_s); i++)
    {
      if (mpu_armv8m_region_enable_check(&dev_mpu_s,
        (struct mpu_armv8m_region_cfg_t *)&region_cfg_loader_s[i]) != MPU_ARMV8M_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, region_cfg_loader_s[i].flow_step_check,
                          region_cfg_loader_s[i].flow_ctrl_check);
      }
    }

    /* Lock MPU config */
    __HAL_RCC_SBS_CLK_ENABLE();
    SBS->CSLCKR |= SBS_CSLCKR_LOCKSMPU;
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_MPU_L_LCK, FLOW_CTRL_MPU_L_LCK);
    if (((* (uint32_t *)read_reg) & SBS_CSLCKR_LOCKSMPU) == 0U)
    {
      Error_Handler();
    }
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_MPU_L_LCK_CH, FLOW_CTRL_MPU_L_LCK_CH);
  }
  
#endif  
#endif /* OEMIROT_MPU_PROTECTION */
}
#endif /* MCUBOOT_EXT_LOADER */


/**
  * @brief  configure flash privilege access
  * @param  None
  * @retval None
  */
static void flash_priv_cfg(void)
{
#ifdef OEMIROT_FLASH_PRIVONLY_ENABLE
  /* configuration stage */
  if (uFlowStage == FLOW_STAGE_CFG)
  {
    /* Configure Flash Privilege access */
    HAL_FLASHEx_ConfigPrivMode(FLASH_NSPRIV_DENIED);
    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_FLASH_P_EN, FLOW_CTRL_FLASH_P_EN);
  }
  /* verification stage */
  else
  {
    if (HAL_FLASHEx_GetPrivMode() != FLASH_NSPRIV_DENIED)
    {
      Error_Handler();
    }
    else
    {
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_FLASH_P_CH, FLOW_CTRL_FLASH_P_CH);
    }
  }
#endif /*  OEMIROT_FLASH_PRIVONLY_ENABLE */
}



#if (OEMIROT_TAMPER_ENABLE != NO_TAMPER)
const RTC_SecureStateTypeDef TamperSecureConf = {
    .rtcSecureFull = RTC_SECURE_FULL_NO,
    .rtcNonSecureFeatures = RTC_NONSECURE_FEATURE_ALL,
    .tampSecureFull = TAMP_SECURE_FULL_YES,
    .MonotonicCounterSecure = TAMP_MONOTONIC_CNT_SECURE_NO,
    .backupRegisterStartZone2 = 0,
    .backupRegisterStartZone3 = 0
};
const RTC_PrivilegeStateTypeDef TamperPrivConf = {
    .rtcPrivilegeFull = RTC_PRIVILEGE_FULL_NO,
    .rtcPrivilegeFeatures = RTC_PRIVILEGE_FEATURE_NONE,
    .tampPrivilegeFull = TAMP_PRIVILEGE_FULL_YES,
    .MonotonicCounterPrivilege = TAMP_MONOTONIC_CNT_PRIVILEGE_NO,
    .backupRegisterStartZone2 = 0,
    .backupRegisterStartZone3 = 0
};
const RTC_InternalTamperTypeDef InternalTamperConf = {
    .IntTamper = RTC_INT_TAMPER_9 | RTC_INT_TAMPER_15,
    .TimeStampOnTamperDetection = RTC_TIMESTAMPONTAMPERDETECTION_DISABLE,
    .NoErase                  = RTC_TAMPER_ERASE_BACKUP_ENABLE
};
void TAMP_IRQHandler(void)
{
    NVIC_SystemReset();
}
#ifdef OEMIROT_DEV_MODE
extern volatile uint32_t TamperEventCleared;
#endif
#endif /* (OEMIROT_TAMPER_ENABLE != NO_TAMPER) */
RTC_HandleTypeDef RTCHandle;

static void active_tamper(void)
{
#if (OEMIROT_TAMPER_ENABLE == ALL_TAMPER)
    RTC_ActiveTampersTypeDef sAllTamper;
    /*  use random generator to feed  */
    uint32_t Seed[4]={0,0,0,0};
    uint32_t len=0;
    uint32_t j;
#endif /* (OEMIROT_TAMPER_ENABLE == ALL_TAMPER) */
#if (OEMIROT_TAMPER_ENABLE != NO_TAMPER)
    RTC_SecureStateTypeDef TamperSecureConfGet;
    RTC_PrivilegeStateTypeDef TamperPrivConfGet;
    fih_int fih_rc = FIH_FAILURE;
#endif /* OEMIROT_TAMPER_ENABLE != NO_TAMPER) */
    /* configuration stage */
    if (uFlowStage == FLOW_STAGE_CFG)
    {
#if defined(OEMIROT_DEV_MODE) && (OEMIROT_TAMPER_ENABLE != NO_TAMPER)
        if (TamperEventCleared) {
            BOOT_LOG_INF("Boot with TAMPER Event Active");
#if (OEMIROT_TAMPER_ENABLE == ALL_TAMPER)
            /* avoid several re-boot in DEV_MODE with Tamper active */
            BOOT_LOG_INF("Plug the tamper cable, and reboot");
            BOOT_LOG_INF("Or");
#endif
            BOOT_LOG_INF("Build and Flash with flag #define OEMIROT_TAMPER_ENABLE NO_TAMPER\n");
            Error_Handler();
        }
#endif /*  OEMIROT_DEV_MODE && (OEMIROT_TAMPER_ENABLE != NO_TAMPER) */

        /* RTC Init */
        RTCHandle.Instance = RTC;
        RTCHandle.Init.HourFormat     = RTC_HOURFORMAT_12;
        RTCHandle.Init.AsynchPrediv   = RTC_ASYNCH_PREDIV;
        RTCHandle.Init.SynchPrediv    = RTC_SYNCH_PREDIV;
        RTCHandle.Init.OutPut         = RTC_OUTPUT_DISABLE;
        RTCHandle.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
        RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        RTCHandle.Init.OutPutType     = RTC_OUTPUT_TYPE_PUSHPULL;
        RTCHandle.Init.OutPutPullUp   = RTC_OUTPUT_PULLUP_NONE;

        if (HAL_RTC_Init(&RTCHandle) != HAL_OK)
        {
            Error_Handler();
        }
#if (OEMIROT_TAMPER_ENABLE == ALL_TAMPER)
        /* generate random seed */
        mbedtls_hardware_poll(NULL, (unsigned char *)Seed, sizeof(Seed),(size_t *)&len);
        if (len == 0)
        {
            Error_Handler();
        }
        BOOT_LOG_INF("TAMPER SEED [0x%lx,0x%lx,0x%lx,0x%lx]", Seed[0], Seed[1], Seed[2], Seed[3]);
        /* Configure active tamper common parameters  */
        sAllTamper.ActiveFilter = RTC_ATAMP_FILTER_ENABLE;
        sAllTamper.ActiveAsyncPrescaler = RTC_ATAMP_ASYNCPRES_RTCCLK_32;
        sAllTamper.TimeStampOnTamperDetection = RTC_TIMESTAMPONTAMPERDETECTION_ENABLE;
        sAllTamper.ActiveOutputChangePeriod = 4;
        sAllTamper.Seed[0] = Seed[0];
        sAllTamper.Seed[1] = Seed[1];
        sAllTamper.Seed[2] = Seed[2];
        sAllTamper.Seed[3] = Seed[3];

        /* Disable all Active Tampers */
        /* No active tamper */
        for (j = 0; j < RTC_TAMP_NB; j++)
        {
            sAllTamper.TampInput[j].Enable = RTC_ATAMP_DISABLE;
        }
        sAllTamper.TampInput[7].Enable = RTC_ATAMP_ENABLE;
        sAllTamper.TampInput[7].Output = 7;
        sAllTamper.TampInput[7].NoErase =  RTC_TAMPER_ERASE_BACKUP_ENABLE;
        sAllTamper.TampInput[7].MaskFlag = RTC_TAMPERMASK_FLAG_DISABLE;
        sAllTamper.TampInput[7].Interrupt = RTC_ATAMP_INTERRUPT_ENABLE;
        /* Set active tampers */
        if (HAL_RTCEx_SetActiveTampers(&RTCHandle, &sAllTamper) != HAL_OK)
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_ACT_EN, FLOW_CTRL_TAMP_ACT_EN);
#else
        HAL_RTCEx_DeactivateTamper(&RTCHandle, RTC_TAMPER_ALL);
#endif  /* (OEMIROT_TAMPER_ENABLE == ALL_TAMPER) */
#if (OEMIROT_TAMPER_ENABLE != NO_TAMPER)
        /*  Internal Tamper activation  */
        /*  Enable Cryptographic IPs fault (tamp_itamp9), Backup domain voltage threshold monitoring (tamp_itamp1)*/
        if (HAL_RTCEx_SetInternalTamper(&RTCHandle,(RTC_InternalTamperTypeDef *)&InternalTamperConf)!=HAL_OK)
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_INT_EN, FLOW_CTRL_TAMP_INT_EN);

        /*  Set tamper configuration secure only  */
        if (HAL_RTCEx_SecureModeSet(&RTCHandle, (RTC_SecureStateTypeDef *)&TamperSecureConf) != HAL_OK)
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_SEC_EN, FLOW_CTRL_TAMP_SEC_EN);

        /*  Set tamper configuration privileged only   */
        if (HAL_RTCEx_PrivilegeModeSet(&RTCHandle,(RTC_PrivilegeStateTypeDef *)&TamperPrivConf) != HAL_OK)
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_PRIV_EN, FLOW_CTRL_TAMP_PRIV_EN);

        /*  Activate Secret Erase */
        HAL_RTCEx_Erase_SecretDev_Conf(&RTCHandle,(uint32_t)TAMP_SECRETDEVICE_ERASE_BKP_SRAM);
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_CFG_EN, FLOW_CTRL_TAMP_CFG_EN);
        BOOT_LOG_INF("TAMPER Activated");
#else
        HAL_RTCEx_DeactivateInternalTamper(&RTCHandle, RTC_INT_TAMPER_ALL);
#endif /* (OEMIROT_TAMPER_ENABLE != NO_TAMPER) */
    }
#if (OEMIROT_TAMPER_ENABLE != NO_TAMPER)
    /* verification stage */
    else
    {
#if (OEMIROT_TAMPER_ENABLE == ALL_TAMPER)
        /* Check active tampers */
        if ((READ_BIT(TAMP->ATOR, TAMP_ATOR_INITS) == 0U) ||
            (READ_REG(TAMP->IER) != (TAMP_IER_TAMP1IE<<7)) ||
            (READ_REG(TAMP->ATCR1) != 0x84050080U) ||
            (READ_REG(TAMP->ATCR2) != TAMP_ATCR2_ATOSEL8) ||
            (READ_REG(TAMP->CR1) != (0x41000000U | (TAMP_CR1_TAMP1E<<7))) ||
            (READ_REG(TAMP->CR2) != 0x00000000U))
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_ACT_CH, FLOW_CTRL_TAMP_ACT_CH);
#endif  /* (OEMIROT_TAMPER_ENABLE == ALL_TAMPER) */
        /*  Check Internal Tamper activation */
        if ((READ_BIT(RTC->CR, RTC_CR_TAMPTS) != InternalTamperConf.TimeStampOnTamperDetection) ||
#if (OEMIROT_TAMPER_ENABLE == ALL_TAMPER)
            (READ_REG(TAMP->CR1) != (0x41000000U | (TAMP_CR1_TAMP1E<<7))) ||
#else
            (READ_REG(TAMP->CR1) != 0x41000000U) ||
#endif /* (OEMIROT_TAMPER_ENABLE == ALL_TAMPER) */
            (READ_REG(TAMP->CR3) != 0x00000000U))
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_INT_CH, FLOW_CTRL_TAMP_INT_CH);

        /*  Check tamper configuration secure only  */
        if (HAL_RTCEx_SecureModeGet(&RTCHandle, (RTC_SecureStateTypeDef *)&TamperSecureConfGet) != HAL_OK)
        {
            Error_Handler();
        }
        FIH_CALL(boot_fih_memequal, fih_rc,(void *)&TamperSecureConf, (void *)&TamperSecureConfGet, sizeof(TamperSecureConf));
        if (fih_not_eq(fih_rc, FIH_SUCCESS)) {
                Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_SEC_CH, FLOW_CTRL_TAMP_SEC_CH);

        /*  Check tamper configuration privileged only   */
        if (HAL_RTCEx_PrivilegeModeGet(&RTCHandle,(RTC_PrivilegeStateTypeDef *)&TamperPrivConfGet) != HAL_OK)
        {
            Error_Handler();
        }
        FIH_CALL(boot_fih_memequal, fih_rc,(void *)&TamperPrivConf, (void *)&TamperPrivConfGet, sizeof(TamperPrivConf));
        if (fih_not_eq(fih_rc, FIH_SUCCESS)) {
                Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_PRIV_CH, FLOW_CTRL_TAMP_PRIV_CH);

        /*  Check Secret Erase */
        if (READ_BIT(TAMP->ERCFGR, TAMP_ERCFGR_ERCFG0) != TAMP_ERCFGR_ERCFG0)
        {
            Error_Handler();
        }
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMP_CFG_CH, FLOW_CTRL_TAMP_CFG_CH);
    }
#endif /*  OEMIROT_TAMPER_ENABLE != NO_TAMPER */
}
/**
  * @}
  */
