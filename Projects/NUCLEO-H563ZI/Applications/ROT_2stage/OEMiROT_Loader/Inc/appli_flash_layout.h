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

#define TRAILER_MAGIC_SIZE 16

#define MCUBOOT_OVERWRITE_ONLY (1)

#define FLASH_B_SIZE                    0x100000
#define FLASH_TOTAL_SIZE                (FLASH_B_SIZE+FLASH_B_SIZE) /* 1 MBytes  or 2MBytes*/

#define OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET     0x18000
#define OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET   0x30000
#define OEMuROT_IMAGE_PARTITION_SIZE               0x18000

#define APP_IMAGE_PRIMARY_PARTITION_OFFSET     0x48000
#define APP_IMAGE_SECONDARY_PARTITION_OFFSET   0x5E000
#define APP_IMAGE_PARTITION_SIZE               0x16000

#define DATA_IMAGE_PRIMARY_PARTITION_OFFSET     0x0
#define DATA_IMAGE_SECONDARY_PARTITION_OFFSET   0x0
#define DATA_IMAGE_PARTITION_SIZE               0x0

// #if !defined(MCUBOOT_OVERWRITE_ONLY)
// /* Flash Driver Used to Confirm Secure App Image */
// #define  FLASH_PRIMARY_SECURE_DEV_NAME             Driver_FLASH0
// #endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_APP_IMAGE_NUMBER == 2) */
// #if !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
// /* Flash Driver Used to Confirm Secure Data Image */
// #define  FLASH_PRIMARY_DATA_SECURE_DEV_NAME        Driver_FLASH0
// #endif /* !defined(MCUBOOT_OVERWRITE_ONLY) && (MCUBOOT_S_DATA_IMAGE_NUMBER == 1) */

#define LOADER_FLASH_DEV_NAME                             Driver_FLASH0


#endif /* __FLASH_LAYOUT_H__ */
