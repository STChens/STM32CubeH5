/* ########### HEADER GENERATED AUTOMATICALLY DONT TOUCH IT ########### */

/*
 * Copyright (c) 2018 Arm Limited. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __APPLI_FLASH_LAYOUT_H__
#define __APPLI_FLASH_LAYOUT_H__

#include "flash_layout.h"

#define TRAILER_MAGIC_SIZE 16

#define IMAGE_PRIMARY_PARTITION_OFFSET     FLASH_AREA_0_OFFSET
#define IMAGE_SECONDARY_PARTITION_OFFSET   FLASH_AREA_2_OFFSET
#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
#define DATA_IMAGE_PRIMARY_PARTITION_OFFSET     FLASH_AREA_4_OFFSET
#define DATA_IMAGE_SECONDARY_PARTITION_OFFSET   FLASH_AREA_6_OFFSET
#endif /* MCUBOOT_S_DATA_IMAGE_NUMBER == 1 */

#if !defined(MCUBOOT_OVERWRITE_ONLY)
/* Flash Driver Used to Confirm Secure App Image */
#define  FLASH_PRIMARY_SECURE_DEV_NAME             Driver_FLASH0
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 2) */
#if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
/* Flash Driver Used to Confirm Secure Data Image */
#define  FLASH_PRIMARY_DATA_SECURE_DEV_NAME        Driver_FLASH0
#endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */

#define FLASH_DEV_NAME                             Driver_FLASH0


#endif /* __FLASH_LAYOUT_H__ */
