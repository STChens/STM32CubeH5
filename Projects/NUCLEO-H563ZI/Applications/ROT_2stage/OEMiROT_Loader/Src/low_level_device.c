/**
  ******************************************************************************
  * @file    low_level_device.c
  * @author  MCD Application Team
  * @brief   This file contains device definition for low_level_device
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
#include "appli_flash_layout.h"
#include "low_level_flash.h"
static struct flash_range access_vect[] =
{
#if !defined(MCUBOOT_PRIMARY_ONLY)
  
  { OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET, OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET + OEMuROT_IMAGE_PARTITION_SIZE - 1},
  { APP_IMAGE_SECONDARY_PARTITION_OFFSET, APP_IMAGE_SECONDARY_PARTITION_OFFSET + APP_IMAGE_PARTITION_SIZE - 1},
#if (DATA_IMAGE_PARTITION_SIZE > 0)
  { DATA_IMAGE_SECONDARY_PARTITION_OFFSET, DATA_IMAGE_SECONDARY_PARTITION_OFFSET + DATA_IMAGE_PARTITION_SIZE - 1},
#endif /* (DATA_IMAGE_PARTITION_SIZE > 0) */
  
#else  /* !defined(MCUBOOT_PRIMARY_ONLY) */
  
  { OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET, OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET + OEMuROT_IMAGE_PARTITION_SIZE - 1},
  { APP_IMAGE_PRIMARY_PARTITION_OFFSET, APP_IMAGE_PRIMARY_PARTITION_OFFSET + APP_IMAGE_PARTITION_SIZE - 1},
#if (DATA_IMAGE_PARTITION_SIZE > 0)
  { DATA_IMAGE_PRIMARY_PARTITION_OFFSET, DATA_IMAGE_PRIMARY_PARTITION_OFFSET + DATA_IMAGE_PARTITION_SIZE - 1},
#endif /* (DATA_IMAGE_PARTITION_SIZE > 0) */
  
#endif /* !defined(MCUBOOT_PRIMARY_ONLY) */
};
#if defined(MCUBOOT_OVERWRITE_ONLY)
#define write_vect access_vect
#else
static struct flash_range write_vect[] =
{
#if !defined(MCUBOOT_PRIMARY_ONLY)
  
  { OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET, OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET + OEMuROT_IMAGE_PARTITION_SIZE - 1},
  { OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET + OEMuROT_IMAGE_PARTITION_SIZE - 32, OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET + OEMuROT_IMAGE_PARTITION_SIZE - 16 - 1},
  { APP_IMAGE_SECONDARY_PARTITION_OFFSET, APP_IMAGE_SECONDARY_PARTITION_OFFSET + APP_IMAGE_PARTITION_SIZE - 1},
  { APP_IMAGE_PRIMARY_PARTITION_OFFSET + APP_IMAGE_PARTITION_SIZE - 32, APP_IMAGE_PRIMARY_PARTITION_OFFSET + APP_IMAGE_PARTITION_SIZE - 16 - 1},
#if (DATA_IMAGE_PARTITION_SIZE > 0)
  { DATA_IMAGE_SECONDARY_PARTITION_OFFSET, DATA_IMAGE_SECONDARY_PARTITION_OFFSET + DATA_IMAGE_PARTITION_SIZE - 1},
  { DATA_IMAGE_PRIMARY_PARTITION_OFFSET + DATA_IMAGE_PARTITION_SIZE - 32, DATA_IMAGE_PRIMARY_PARTITION_OFFSET + DATA_IMAGE_PARTITION_SIZE - 16 - 1},
#endif /* (DATA_IMAGE_PARTITION_SIZE > 0) */
  
#else /* !defined(MCUBOOT_PRIMARY_ONLY) */
  
  { OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET, OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET + OEMuROT_IMAGE_PARTITION_SIZE - 1},  
  { APP_IMAGE_PRIMARY_PARTITION_OFFSET, APP_IMAGE_PRIMARY_PARTITION_OFFSET + APP_IMAGE_PARTITION_SIZE - 1},  
#if (DATA_IMAGE_PARTITION_SIZE > 0)
  { DATA_IMAGE_PRIMARY_PARTITION_OFFSET, DATA_IMAGE_PRIMARY_PARTITION_OFFSET + DATA_IMAGE_PARTITION_SIZE - 1},  
#endif /* (DATA_IMAGE_PARTITION_SIZE > 0) */
  
#endif  /* !defined(MCUBOOT_PRIMARY_ONLY) */

};
#endif /* defined(MCUBOOT_OVERWRITE_ONLY) */
struct low_level_device FLASH0_DEV =
{
  .erase = { .nb = sizeof(access_vect) / sizeof(struct flash_range), .range = access_vect},
  .write = { .nb = sizeof(write_vect) / sizeof(struct flash_range), .range = write_vect},
};
