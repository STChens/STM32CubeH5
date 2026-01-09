/**
  ******************************************************************************
  * @file    low_level_obkeys.c
  * @author  MCD Application Team
  * @brief   Low Level Interface module to access OKB area in FLASH
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
#include "stm32h5xx_hal.h"
#include "low_level_obkeys.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
#define ARM_FLASH_API_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2,1)  /* API version */


#define _ARM_Driver_Flash_(n)      Driver_Flash##n
#define  ARM_Driver_Flash_(n) _ARM_Driver_Flash_(n)


#define ARM_FLASH_SECTOR_INFO(addr,size) { (addr), (addr)+(size)-1 }

/**
\brief Flash Sector information
*/
typedef struct _ARM_FLASH_SECTOR {
  uint32_t start;                       ///< Sector Start address
  uint32_t end;                         ///< Sector End address (start+size-1)
} const ARM_FLASH_SECTOR;

/**
\brief Flash information
*/
typedef struct _ARM_FLASH_INFO {
  ARM_FLASH_SECTOR *sector_info;        ///< Sector layout information (NULL=Uniform sectors)
  uint32_t          sector_count;       ///< Number of sectors
  uint32_t          sector_size;        ///< Uniform sector size in bytes (0=sector_info used) 
  uint32_t          page_size;          ///< Optimal programming page size in bytes
  uint32_t          program_unit;       ///< Smallest programmable unit in bytes
  uint8_t           erased_value;       ///< Contents of erased memory (usually 0xFF)
} const ARM_FLASH_INFO;


/**
\brief Flash Status
*/
typedef volatile struct _ARM_FLASH_STATUS {
  uint32_t busy     : 1;                ///< Flash busy flag
  uint32_t error    : 1;                ///< Read/Program/Erase error flag (cleared on start of next operation)
  uint32_t reserved : 30;
} ARM_FLASH_STATUS;


/****** Flash Event *****/
#define ARM_FLASH_EVENT_READY           (1UL << 0)  ///< Flash Ready
#define ARM_FLASH_EVENT_ERROR           (1UL << 1)  ///< Read/Program/Erase Error


/**
  * Arm Flash device structure.
  */
struct arm_obk_flash_dev_t
{
  OBK_LowLevelDevice *dev;
  ARM_FLASH_INFO *data;       /*!< OBK FLASH memory device data */
};

/* Private defines -----------------------------------------------------------*/
#define SBS_EXT_EPOCHSELCR_EPOCH_SEL_S_EPOCH (1U << SBS_EPOCHSELCR_EPOCH_SEL_Pos )
#ifdef OEMUROT_ENABLE
#define MAX_SIZE_CFG                         OBK_HDPL2_CFG_SIZE
#else
#define MAX_SIZE_CFG                         OBK_HDPL1_CFG_SIZE
#endif
#define ST_SHA256_TIMEOUT                       (3U)

/* config for OBK flash driver */
#define OBK_FLASH0_TOTAL_SIZE                   (0x2000U)
#define OBK_FLASH0_PROG_UNIT                    (OBK_FLASH_PROG_UNIT)
#define OBK_FLASH0_ERASED_VAL                   (0xFF)

/* Private variables ---------------------------------------------------------*/
static HASH_HandleTypeDef hhash;
static ARM_FLASH_INFO ARM_OBK_FLASH0_DEV_DATA =
{
  .sector_info    = NULL,     /* Uniform sector layout */
  .sector_count   = OBK_FLASH0_TOTAL_SIZE / OBK_FLASH0_PROG_UNIT,
  .sector_size    = OBK_FLASH0_PROG_UNIT,
  .page_size      = OBK_FLASH0_PROG_UNIT,
  .program_unit   = OBK_FLASH0_PROG_UNIT, /* Minimum write size in bytes */
  .erased_value   = OBK_FLASH0_ERASED_VAL
};

static struct arm_obk_flash_dev_t ARM_OBK_FLASH0_DEV =
{
  .dev    = &(OBK_FLASH0_DEV),
  .data   = &(ARM_OBK_FLASH0_DEV_DATA)
};

/* Global variables ----------------------------------------------------------*/
#if defined(__ICCARM__)
#pragma location=".ram_init_ro"
#elif defined(__GNUC__)
__attribute__((section(".ram_init_ro")))
#endif /* __ICCARM__ */

OBK_Hdpl1Config OBK_Hdpl1_Cfg;
__IO uint32_t DoubleECC_Error_Counter;

#define true 1
#define false 0
/* Private function prototypes -----------------------------------------------*/
int32_t OBK_Flash_WriteEncrypted(uint32_t Offset, const void *pData, uint32_t Length);
/* Functions Definition ------------------------------------------------------*/
/**
  * \brief      Check if the Flash memory boundaries are not violated.
  * \param[in]  flash_dev  Flash device structure \ref arm_obk_flash_dev_t
  * \param[in]  offset     Highest Flash memory address which would be accessed.
  * \return     Returns true if Flash memory boundaries are not violated, false
  *             otherwise.
  */
static int is_range_valid(struct arm_obk_flash_dev_t *Flash_dev, uint32_t Offset)
{
  return (Offset <= OBK_HDPL1_END) ? (true) : (false);
}

/**
  * \brief  Check if the parameter is aligned to program_unit.
  * @parma  Param Any number that can be checked against the
  *               program_unit, e.g. Flash memory address or
  *               data length in bytes.
  * @retval Returns true if param is aligned to program_unit, false
  *               otherwise.
  */
static int is_write_aligned(struct arm_obk_flash_dev_t *Flash_dev, uint32_t Param)
{
  return ((Param % Flash_dev->data->program_unit) != 0U) ? (false) : (true);
}

/**
  * @brief  Control if the length is 16 bytes multiple (QUADWORD)
  * @param  Length: Number of bytes (multiple of 16 bytes)
  * @retval None
  */
static int is_write_allowed(struct arm_obk_flash_dev_t *Flash_dev, uint32_t Length)
{
  return ((Length % Flash_dev->data->program_unit) != 0U) ? (false) : (true);
}

/**
  * @brief  Read encrypted OBkeys
  * @param  Offset Offset in the OBKeys area (aligned on 16 bytes)
  * @param  pData Data buffer to be filled (aligned on 4 bytes)
  * @param  Length Number of bytes (multiple of 4 bytes)
  * @retval ARM_DRIVER error status
  */
static int32_t OBK_Flash_ReadEncrypted(uint32_t Offset, void *pData, uint32_t Length)
{
#if defined(BL2_HW_ACCEL_ENABLE)
  CRYP_HandleTypeDef hcryp = { 0U };
  uint32_t SaesTimeout = 100U;
  uint32_t DataEncrypted[MAX_SIZE_CFG / 4U] = { 0UL };
#endif /* BL2_HW_ACCEL_ENABLE */
  uint8_t *p_source = (uint8_t *) (FLASH_OBK_BASE_S + Offset);
#if defined(BL2_HW_ACCEL_ENABLE)
  uint8_t *p_destination = (uint8_t *) DataEncrypted;
  uint32_t a_aes_iv[4] = {0x8001D1CEU, 0xD1CED1CEU, 0xD1CE8001U, 0xCED1CED1U};
#else
  uint8_t *p_destination = (uint8_t *) pData;
#endif /* BL2_HW_ACCEL_ENABLE */

  /* Check OBKeys  boundaries */
  if (is_range_valid(&ARM_OBK_FLASH0_DEV, Offset + Length -1U) != true)
  {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  /* Do not use memcpy from lib to manage properly ECC error */
  DoubleECC_Error_Counter = 0U;
  memcpy(p_destination, p_source, Length);
  if (DoubleECC_Error_Counter != 0U)
  {
    printf("Double ECC error detected: FLASH_ECCDETR=0x%lx\r\n", FLASH->ECCDETR);
    memset(p_destination, 0x00, Length);
  }

#if defined(BL2_HW_ACCEL_ENABLE)
  __HAL_RCC_SBS_CLK_ENABLE();
  __HAL_RCC_SAES_CLK_ENABLE();

  /* Force use of EPOCH_S value for DHUK */
  WRITE_REG(SBS_S->EPOCHSELCR, SBS_EXT_EPOCHSELCR_EPOCH_SEL_S_EPOCH);

  /* Configure SAES parameters */
  hcryp.Instance = SAES_S;
  if (HAL_CRYP_DeInit(&hcryp) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }
  hcryp.Init.DataType  = CRYP_NO_SWAP;
  hcryp.Init.KeySelect = CRYP_KEYSEL_HW;  /* Hardware unique key (256-bits) */
  hcryp.Init.Algorithm = CRYP_AES_CBC;
  hcryp.Init.KeyMode = CRYP_KEYMODE_NORMAL ;
  hcryp.Init.KeySize = CRYP_KEYSIZE_256B; /* 256 bits AES Key*/
  hcryp.Init.pInitVect = a_aes_iv;

  if (HAL_CRYP_Init(&hcryp) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }

  /*Size is n words*/
  if (HAL_CRYP_Decrypt(&hcryp, (uint32_t *)&DataEncrypted[0U], (uint16_t) (Length / 4U), (uint32_t *)pData, SaesTimeout) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }
  if (HAL_CRYP_DeInit(&hcryp) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }
#endif /* BL2_HW_ACCEL_ENABLE */

  return ARM_DRIVER_OK;
}

/**
  * @brief  Write encrypted OBkeys
  * @param  Offset Offset in the OBKeys area (aligned on 16 bytes)
  * @param  pData Data buffer to be programmed encrypted (aligned on 4 bytes)
  * @param  Length Number of bytes (multiple of 16 bytes)
  * @retval ARM_DRIVER error status
  */
int32_t OBK_Flash_WriteEncrypted(uint32_t Offset, const void *pData, uint32_t Length)
{
  uint32_t i = 0U;
  uint32_t destination = FLASH_OBK_BASE_S + Offset;
  FLASH_EraseInitTypeDef FLASH_EraseInitStruct = {0U};
  uint32_t sector_error = 0U;
#if defined(BL2_HW_ACCEL_ENABLE)
  CRYP_HandleTypeDef hcryp = {0U};
  uint32_t SaesTimeout = 100U;
  uint32_t DataEncrypted[MAX_SIZE_CFG / 4U] = {0UL};
  uint32_t a_aes_iv[4] = {0x8001D1CEU, 0xD1CED1CEU, 0xD1CE8001U, 0xCED1CED1U};
#endif /* BL2_HW_ACCEL_ENABLE */

  /* Check parameters */
  if ((is_range_valid(&ARM_OBK_FLASH0_DEV, Offset + Length - 1U) != true) ||
      (is_write_aligned(&ARM_OBK_FLASH0_DEV, Offset) != true) ||
      (is_write_allowed(&ARM_OBK_FLASH0_DEV, Length) != true) ||
      (Length > MAX_SIZE_CFG))
  {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

#if defined(BL2_HW_ACCEL_ENABLE)
  __HAL_RCC_SBS_CLK_ENABLE();
  __HAL_RCC_SAES_CLK_ENABLE();
#endif /* BL2_HW_ACCEL_ENABLE */

  /* Unlock  Flash area */
  (void) HAL_FLASH_Unlock();
  (void) HAL_FLASHEx_OBK_Unlock();

#if defined(BL2_HW_ACCEL_ENABLE)
  /* Force use of EPOCH_S value for DHUK */
  WRITE_REG(SBS_S->EPOCHSELCR, SBS_EXT_EPOCHSELCR_EPOCH_SEL_S_EPOCH);

  /* Configure SAES parameters */
  hcryp.Instance = SAES_S;
  if (HAL_CRYP_DeInit(&hcryp) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }
  hcryp.Init.DataType = CRYP_NO_SWAP;
  hcryp.Init.KeySelect = CRYP_KEYSEL_HW;        /* Hardware key : derived hardware unique key (DHUK 256-bit) */
  hcryp.Init.Algorithm = CRYP_AES_CBC;
  hcryp.Init.KeyMode = CRYP_KEYMODE_NORMAL ;
  hcryp.Init.KeySize = CRYP_KEYSIZE_256B;       /* 256 bits AES Key */
  hcryp.Init.pInitVect = a_aes_iv;

  if (HAL_CRYP_Init(&hcryp) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }

  /* Size is n words */
  if (HAL_CRYP_Encrypt(&hcryp, (uint32_t *)pData, (uint16_t) (Length / 4U), &DataEncrypted[0U], SaesTimeout) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }
  if (HAL_CRYP_DeInit(&hcryp) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }
#endif /* BL2_HW_ACCEL_ENABLE */

  /* Erase OBKeys */
  FLASH_EraseInitStruct.TypeErase = FLASH_TYPEERASE_OBK_ALT;
  if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &sector_error) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }

  /* Program OBKeys */
  for (i = 0U; i < Length; i += OBK_FLASH_PROG_UNIT)
  {
#if defined(BL2_HW_ACCEL_ENABLE)
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD_OBK_ALT, (destination + i), (uint32_t)&DataEncrypted[i / 4U]) != HAL_OK)
#else
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD_OBK_ALT, (destination + i), (uint32_t)&((uint32_t*)pData)[i / 4U]) != HAL_OK)
#endif /* BL2_HW_ACCEL_ENABLE */
    {
      return ARM_DRIVER_ERROR_SPECIFIC;
    }
  }

  /* Swap all OBKeys */
  if (HAL_FLASHEx_OBK_Swap(ALL_OBKEYS) != HAL_OK)
  {
      return ARM_DRIVER_ERROR_SPECIFIC;
  }

  /* Lock the User Flash area */
  (void) HAL_FLASH_Lock();
  (void) HAL_FLASHEx_OBK_Lock();

  return ARM_DRIVER_OK;
}

/**
  * @brief  Read non-encrypted OBkeys
  * @param  Offset: Offset in the OBKeys area (aligned on 16 bytes)
  * @param  pData Data buffer to be filled (aligned on 4 bytes)
  * @param  Length: Number of bytes (multiple of 4 bytes)
  * @retval ARM_DRIVER error status
  */
static int32_t OBK_Read(uint32_t Offset, void *pData, uint32_t Length)
{
  uint8_t *p_source = (uint8_t *) (FLASH_OBK_BASE_S + Offset);
  uint8_t *p_destination = (uint8_t *) pData;

  /* Check parameters */
  if (is_range_valid(&ARM_OBK_FLASH0_DEV, Offset + Length - 1U) != true)
  {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  /* Do not use memcpy from lib to manage properly ECC error */
  DoubleECC_Error_Counter = 0U;
  memcpy(p_destination, p_source, Length);
  if (DoubleECC_Error_Counter != 0U)
  {
    printf("Double ECC error detected: FLASH_ECCDETR=0x%lx\r\n", FLASH->ECCDETR);
    memset(p_destination, 0x00, Length);
  }

  return ARM_DRIVER_OK;
}

/**
  * @brief  Write OBkeys
  * @param  Offset: Offset in the OBKeys area (aligned on 16 bytes)
  * @param  Length: Number of bytes (multiple of 4 bytes)
  * @param  DataAddress Data buffer to be encrypted (aligned on 4 bytes)
  * @retval ARM_DRIVER error status
  */
static int32_t OBK_Write(uint32_t Offset, const void *pData, uint32_t Length)
{
  uint32_t i = 0U;
  uint32_t destination  = FLASH_OBK_BASE_S + Offset;
  FLASH_EraseInitTypeDef FLASH_EraseInitStruct = { 0U };;
  uint32_t sector_error = 0U;
  uint32_t data_address = (uint32_t) pData;

  /* Check parameters */
  if ((is_range_valid(&ARM_OBK_FLASH0_DEV, Offset + Length - 1U) != true) ||
      (is_write_aligned(&ARM_OBK_FLASH0_DEV, Offset) != true) ||
      (is_write_allowed(&ARM_OBK_FLASH0_DEV, Length) != true))
  {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  /* Unlock  Flash area */
  (void) HAL_FLASH_Unlock();
  (void) HAL_FLASHEx_OBK_Unlock();

  /* Erase OBKeys */
  FLASH_EraseInitStruct.TypeErase = FLASH_TYPEERASE_OBK_ALT;
  if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &sector_error) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }

  /* Program OBKeys */
  for (i = 0U; i < Length; i += OBK_FLASH_PROG_UNIT)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD_OBK_ALT, (destination + i), (data_address + i)) != HAL_OK)
    {
    return ARM_DRIVER_ERROR_SPECIFIC;
    }
  }

  /* Swap all OBKeys */
  if (HAL_FLASHEx_OBK_Swap(ALL_OBKEYS) != HAL_OK)
  {
    return ARM_DRIVER_ERROR_SPECIFIC;
  }

  /* Lock the User Flash area */
  (void) HAL_FLASH_Lock();
  (void) HAL_FLASHEx_OBK_Lock();

  return ARM_DRIVER_OK;
}

/**
  * @brief  Memory compare with constant time execution.
  * @note   Objective is to avoid basic attacks based on time execution
  * @param  pAdd1 Address of the first buffer to compare
  * @param  pAdd2 Address of the second buffer to compare
  * @param  Size Size of the comparison
  * @retval SFU_ SUCCESS if equal, a SFU_error otherwise.
  */
static uint32_t MemoryCompare(uint8_t *pAdd1, uint8_t *pAdd2, uint32_t Size)
{
  uint8_t result = 0x00U;
  uint32_t i = 0U;

  for (i = 0U; i < Size; i++)
  {
    result |= pAdd1[i] ^ pAdd2[i];
  }
  return result;
}

/**
  * @brief  Compute SHA256
  * @param  pBuffer: pointer to the input buffer to be hashed
  * @param  Length: length of the input buffer in bytes
  * @param  pSHA256: pointer to the compuyed digest
  * @retval None
  */
static HAL_StatusTypeDef Compute_SHA256(uint8_t *pBuffer, uint32_t Length, uint8_t *pSHA256)
{
  /* Enable HASH clock */
  __HAL_RCC_HASH_CLK_ENABLE();

  hhash.Instance = HASH;
  /* HASH Configuration */
  if (HAL_HASH_DeInit(&hhash) != HAL_OK)
  {
    return HAL_ERROR;
  }
  hhash.Init.DataType = HASH_BYTE_SWAP;
  hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA256;
  if (HAL_HASH_Init(&hhash) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* HASH computation */
  if (HAL_HASH_Start(&hhash, pBuffer, Length, pSHA256, ST_SHA256_TIMEOUT) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

/**
  * @brief  Read data in OBkeys HDPL1
  * @param  pOBK_Hdpl1Data: pointer on HDPL1 data
  * @retval None
  */
HAL_StatusTypeDef OBK_ReadHdpl1Data(OBK_Hdpl1Data *pOBK_Hdpl1Data)
{
  uint8_t sha256[SHA256_LENGTH] = { 0U };
  uint32_t Address = (uint32_t) pOBK_Hdpl1Data;

  /* Read configuration in OBKeys */
  if (OBK_Read(OBK_HDPL1_DATA_OFFSET, (void *) pOBK_Hdpl1Data, sizeof(OBK_Hdpl1Data)) != ARM_DRIVER_OK)
  {
    return HAL_ERROR;
  }

  /* Verif SHA256 on the whole Hdpl 1 data except first 32 bytes of SHA256 */
  if (Compute_SHA256((uint8_t *) (Address + SHA256_LENGTH), sizeof(OBK_Hdpl1Data) - SHA256_LENGTH, sha256) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (MemoryCompare(pOBK_Hdpl1Data->SHA256, sha256, SHA256_LENGTH) != 0U)
  {
    printf("Wrong OBK HDPL1 data\r\n");
    return HAL_ERROR;
  }
  return HAL_OK;
}

/**
  * @brief  Update data in OBkeys Hdpl1 1
  * @param  pOBK_Hdpl1Data: pointer on Hdpl 1 data
  * @retval ARM_DRIVER error status
  */
HAL_StatusTypeDef OBK_UpdateHdpl1Data(OBK_Hdpl1Data *pOBK_Hdpl1Data)
{
  uint8_t sha256[SHA256_LENGTH] = { 0U };
  uint32_t Address = (uint32_t) pOBK_Hdpl1Data;

  /* Verif SHA256 on the whole Hdpl 1 data except first 32 bytes of SHA256 */
  if (Compute_SHA256((uint8_t *) (Address + SHA256_LENGTH), sizeof(OBK_Hdpl1Data) - SHA256_LENGTH, sha256) != HAL_OK)
  {
    printf("Wrong OBK HDPL1 data\r\n");
    return HAL_ERROR;
  }
  (void) memcpy(&pOBK_Hdpl1Data->SHA256[0], &sha256[0], SHA256_LENGTH);

  /* Write configuration in OBKeys */
  if (OBK_Write(OBK_HDPL1_DATA_OFFSET, (void *) pOBK_Hdpl1Data, sizeof(OBK_Hdpl1Data)) != ARM_DRIVER_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}


/* Public Functions Definition ------------------------------------------------------*/
extern void Error_Handler(void);
/**
  * @brief  Read configuration in OBkeys Hdpl 1
  * @param  pOBK_Hdpl1Cfg : pointer on Hdpl 1 configuration
  * @retval None
  */
void OBK_ReadHdpl1Config(OBK_Hdpl1Config *pOBK_Hdpl1Cfg)
{
  uint8_t sha256[SHA256_LENGTH] = { 0U };
  uint32_t Address = (uint32_t) pOBK_Hdpl1Cfg;

  /* Read configuration in OBKeys */
  if (OBK_Flash_ReadEncrypted(OBK_HDPL1_CFG_OFFSET,(void *) pOBK_Hdpl1Cfg,  sizeof(OBK_Hdpl1Config)) != ARM_DRIVER_OK)
  {
    Error_Handler();
  }

  /* Verif SHA256 on the whole Hdpl 1 config except first 32 bytes of SHA256 */
  if (Compute_SHA256((uint8_t *) (Address + SHA256_LENGTH), sizeof(OBK_Hdpl1Config) - SHA256_LENGTH, sha256) != HAL_OK)
  {
    Error_Handler();
  }
  if (MemoryCompare(&pOBK_Hdpl1Cfg->SHA256[0], &sha256[0], SHA256_LENGTH) != 0U)
  {
    printf("read: Wrong OBK HDPL1 cfg");
    Error_Handler();
  }
}

/**
  * @brief  Verify configuration in OBkeys Hdpl1 1
  * @param  pOBK_Hdpl1Cfg : pointer on Hdpl 1 configuration
  * @retval None
  */
void OBK_VerifyHdpl1Config(OBK_Hdpl1Config *pOBK_Hdpl1Cfg)
{
  uint8_t sha256[SHA256_LENGTH] = { 0U };
  uint32_t Address = (uint32_t) pOBK_Hdpl1Cfg;

  /* Verif SHA256 on the whole Hdpl 1 config except first 32 bytes of SHA256 */
  if (Compute_SHA256((uint8_t *) (Address + SHA256_LENGTH), sizeof(OBK_Hdpl1Config) - SHA256_LENGTH, sha256) != HAL_OK)
  {
    Error_Handler();
  }
  if (MemoryCompare(&pOBK_Hdpl1Cfg->SHA256[0], &sha256[0], SHA256_LENGTH) != 0U)
  {
    printf("verify: Wrong OBK HDPL1 cfg");
    Error_Handler();
  }
}


/**
  * @brief  Init DHUK
  * @param  pBuffer pointer to the input buffer to be hashed
  * @param  Length length of the input buffer in bytes
  * @param  pSHA256 pointer to the compuyed digest
  * @retval None
  */
void OBK_InitDHUK(void)
{

}

/**
  * @brief  All checks have been done, provision data
  * @param  none
  * @retval None
  */
//const uint32_t Offsets[NB_REGIONS] = { OBK_HDPL1_OFFSET, OBK_HDPL1_CFG_OFFSET, OBK_HDPL1_DATA_OFFSET, OBK_HDPL3_OFFSET };
typedef struct {
    uint32_t addr;
    uint32_t length;
    uint32_t encrypted;
  } OBK_Header_t;

uint32_t OBK_ProvisionDefaultDAObk(int prov)
{
  uint32_t ReadContent[24]; // 96 bytes of DA data
  uint32_t offset, size;
  OBK_Header_t header;
  uint32_t i;
  uint8_t ProvData[] = {  
   0x00, 0x01, 0xfd, 0x0f, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x9c, 0x08, 0xe9, 
   0x69, 0x34, 0xb8, 0xb0, 0xc7, 0x54, 0xe7, 0xb3, 0x87, 0x04, 0xdc, 0xca, 0x96, 0x11, 0x55, 0x18, 
   0x38, 0xf2, 0xf4, 0x29, 0xd3, 0x0f, 0xc0, 0x56, 0x98, 0x6a, 0x75, 0x7f, 0xcc, 0x05, 0xb0, 0x13, 
   0x17, 0x77, 0x5c, 0x86, 0x5a, 0x3f, 0x98, 0x7d, 0x17, 0x78, 0xc1, 0x2d, 0x7e, 0x51, 0x78, 0x51, 
   0xa0, 0xf4, 0x2d, 0xd9, 0x2d, 0xfa, 0x6c, 0x1d, 0x85, 0x9b, 0x25, 0xfe, 0x77, 0x50, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  printf("Read DA OBK before provisioning\r\n");
  memset((void *) ReadContent, 0,  sizeof(ReadContent));
  uint32_t result= OBK_Read(OBK_HDPL1_OFFSET,(void *) ReadContent,  sizeof(ReadContent));

  if (result != ARM_DRIVER_OK)
  {
    printf("OBK_Read error %d\r\n", result);
    Error_Handler();
  }
  printf("\r\n==============================================\r\n");
  for ( i=(uint32_t)&ReadContent[0] ; i <(uint32_t) &ReadContent[0]+sizeof(ReadContent); i+=16)
  {
    printf("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\r\n",
                 *(uint8_t *)(i+0), *(uint8_t *)(i+1), *(uint8_t *)(i+2), *(uint8_t *)(i+3),
                 *(uint8_t *)(i+14), *(uint8_t *)(i+5), *(uint8_t *)(i+6), *(uint8_t *)(i+7),
                 *(uint8_t *)(i+8), *(uint8_t *)(i+9), *(uint8_t *)(i+10), *(uint8_t *)(i+11),
                 *(uint8_t *)(i+12), *(uint8_t *)(i+13), *(uint8_t *)(i+14), *(uint8_t *)(i+15)
                 );
  }
  printf("\r\n==============================================\r\n");
  
  __HAL_RCC_SBS_CLK_ENABLE();

  memcpy(&header, (void*) &ProvData[0], sizeof(header));
  printf("Read OBK header from default DA data\r\n");
  printf("OBK address: %08x\r\n", header.addr);
  printf("OBK length: %08x\r\n", header.length);
  printf("OBK encrypted: %08x\r\n", header.encrypted);
  
  printf("Current SBS OBKHDPL is %x\r\n", HAL_SBS_GetOBKHDPL());
  printf("Current SBS HDPL is %02x \r\n", HAL_SBS_GetHDPLValue());
    
  if(prov == 1)
  {
    if((header.encrypted != 0) && (header.encrypted != 1))
    {
      printf("Wrong Header encrypted value !\r\n");
      Error_Handler();
    }

    if((header.addr & 0xFFFF0000) != 0x0FFD0000UL)
    {
      printf("Wrong address !\r\n");
      Error_Handler();
    }

     // Check we are consistent between expected data
    offset = (header.addr - FLASH_OBK_BASE_S);
    size = header.length;
    
    {
      printf("Provision OBK data not ENCRYPTED\r\n");
      result = OBK_Write(offset, (const void*)(&ProvData[0]+sizeof(OBK_Header_t)), size);
    }
    
    printf("\r\nRead DA OBK after provisioning\r\n");
    result= OBK_Read(OBK_HDPL1_OFFSET,(void *) ReadContent,  sizeof(ReadContent));

    if (result != ARM_DRIVER_OK)
    {
      printf("OBK_Read error %d", result);
      Error_Handler();
    }
    printf("==============================================\r\n");
    for (i=(uint32_t)&ReadContent[0] ; i <(uint32_t) &ReadContent[0]+sizeof(ReadContent); i+=16)
    {
      printf("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\r\n",
                   *(uint8_t *)(i+0), *(uint8_t *)(i+1), *(uint8_t *)(i+2), *(uint8_t *)(i+3),
                   *(uint8_t *)(i+14), *(uint8_t *)(i+5), *(uint8_t *)(i+6), *(uint8_t *)(i+7),
                   *(uint8_t *)(i+8), *(uint8_t *)(i+9), *(uint8_t *)(i+10), *(uint8_t *)(i+11),
                   *(uint8_t *)(i+12), *(uint8_t *)(i+13), *(uint8_t *)(i+14), *(uint8_t *)(i+15)
                   );
    }
    printf("\r\n==============================================\r\n");
  }

  return result;
}

