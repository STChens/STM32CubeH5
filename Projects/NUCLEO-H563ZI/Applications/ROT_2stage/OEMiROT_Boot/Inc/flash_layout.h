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

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

/* This header file is included from linker scatter file as well, where only a
 * limited C constructs are allowed. Therefore it is not possible to include
 * here the platform_retarget.h to access flash related defines. To resolve this
 * some of the values are redefined here with different names, these are marked
 * with comment.
 */

/* Flash layout configuration : begin ****************************************/
#define OEMiROT_OEMUROT_ENABLE         /* Defined: the project is used for OEMiROT_OEMuROT boot path */
/* #define STANDALONE_LOADER */    /* Defined: standalone Loader will be used instead of system bootloader, 
                                 for this flag to take effect, MCUBOOT_EXT_LOADER must be defined */

#define MCUBOOT_OVERWRITE_ONLY     /* Defined: the FW installation uses overwrite method.
                                      UnDefined: The FW installation uses swap mode. */

#define MCUBOOT_EXT_LOADER         /* Defined: Use system bootloader (in system flash).
                                               To enter it, press user button at reset.
                                      Undefined: Do not use system bootloader. */

#define MCUBOOT_APP_IMAGE_NUMBER 1 /* 1: App is OEMuROT which is S only, so the App image must be 1 */

#define MCUBOOT_S_DATA_IMAGE_NUMBER 0   /* App is OEMuROT, no data slot, this must be 0 */
#define MCUBOOT_NS_DATA_IMAGE_NUMBER 0  /* App is OEMuROT, no data slot, this must be 0*/

/*#define DEVICE_1M_FLASH_ENABLE */  /*Defined: the project is for 1M FLASH device
                                       Undefined: the project is for 2M FLASH device */
/* Flash layout configuration : end ******************************************/
#if (MCUBOOT_S_DATA_IMAGE_NUMBER) != 0
#error "MCUBOOT_S_DATA_IMAGE_NUMBER must be 0 (OEMiROT App is OEMuROT"
#endif /* (MCUBOOT_S_DATA_IMAGE_NUMBER ) != 0*/
#if (MCUBOOT_NS_DATA_IMAGE_NUMBER) != 0
#error "MCUBOOT_NS_DATA_IMAGE_NUMBER must be 0 (OEMiROT App is OEMuROT"
#endif /* (MCUBOOT_NS_DATA_IMAGE_NUMBER ) != 0*/
#if (MCUBOOT_APP_IMAGE_NUMBER) != 1
#error "MCUBOOT_APP_IMAGE_NUMBER must be 1 (OEMiROT App is OEMuROT (S only)"
#endif /* MCUBOOT_APP_IMAGE_NUMBER != 1*/


/* Total number of images is 1 for OEMuROT as App*/
#define MCUBOOT_IMAGE_NUMBER 1

/* Use image hash reference to reduce boot time (signature check bypass) */
#define MCUBOOT_USE_HASH_REF

/* control configuration */

/* The size of a partition. This should be large enough to contain a S or NS
 * sw binary. Each FLASH_AREA_IMAGE contains two partitions. See Flash layout
 * above.
 */

/* Flash layout info for BL2 bootloader */
#define FLASH_AREA_IMAGE_SECTOR_SIZE    (0x2000)     /* 8 KB */
#define FLASH_AREA_WRP_GROUP_SIZE       (0x8000)     /* 32 KB */
#if defined(DEVICE_1M_FLASH_ENABLE)
#define FLASH_B_SIZE                    (0x80000)   /* 512 KBytes*/
#else
#define FLASH_B_SIZE                    (0x100000) /* 1 MBytes */
#endif /* DEVICE_1M_FLASH_ENABLE */
#define FLASH_TOTAL_SIZE                (FLASH_B_SIZE+FLASH_B_SIZE) /* 1 MBytes  or 2MBytes*/
#define FLASH_BASE_ADDRESS              (0x08000000)

/* Flash area IDs */
#define FLASH_AREA_0_ID                 (1) /* Active (Primary) slot for OEMuROT as App */
#define FLASH_AREA_2_ID                 (3) /* Download (Secondary) slot for OEMuROT as App */
#define FLASH_AREA_SCRATCH_ID           (9) // Scratch area is needed only in non overwrite mode 

/* Offset and size definitions of the flash partitions that are handled by the
 * bootloader. The image swapping is done between IMAGE_0 and IMAGE_1, SCRATCH
 * is used as a temporary storage during image swapping.
 */

/*--------------------------------
 * 1. BL2 (OEMiROT code area)
 *--------------------------------
 */
/* area for BL2 code protected by hdp */
#define FLASH_AREA_BL2_OFFSET           (0x0000)
#define FLASH_AREA_BL2_SIZE             (0x18000)

/*--------------------------------
 * 2. Loader (Loader code area)
 *--------------------------------
 */
#if defined (MCUBOOT_EXT_LOADER) && defined (STANDALONE_LOADER)
#define FLASH_AREA_LOADER_SIZE             (0x8000)
/* HDP area end at this address, exclude Loader code area */
#define FLASH_BL2_HDP_END               (FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE-FLASH_AREA_LOADER_SIZE-1)
#else
#define FLASH_AREA_LOADER_SIZE             (0x0)
/* HDP area end at this address */
#define FLASH_BL2_HDP_END               (FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE-1)
#endif

#define FLASH_AREA_LOADER_OFFSET           (FLASH_AREA_BL2_OFFSET + FLASH_AREA_BL2_SIZE - FLASH_AREA_LOADER_SIZE)

/* control area under WRP group protection */
#if (FLASH_AREA_BL2_OFFSET % FLASH_AREA_WRP_GROUP_SIZE) != 0
#error "FLASH_AREA_BL2_OFFSET not aligned on FLASH_AREA_WRP_GROUP_SIZE"
#endif /* (FLASH_AREA_BL2_OFFSET % FLASH_AREA_WRP_GROUP_SIZE) != 0 */
#if ((FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE) % FLASH_AREA_WRP_GROUP_SIZE) != 0
#error "(FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE) not aligned on FLASH_AREA_WRP_GROUP_SIZE"
#endif /* ((FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE) % FLASH_AREA_WRP_GROUP_SIZE) != 0 */

/* control area for BL2 code protected by hdp */
#if ((FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE) % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0
#error "HDP area must be aligned on FLASH_AREA_IMAGE_SECTOR_SIZE"
#endif /* ((FLASH_AREA_BL2_OFFSET+FLASH_AREA_BL2_SIZE) % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0 */

/* scratch area */
/*------------------------------------------------------------
 * 3. Scratch slot (if not using OVERWRITE mode)
 * NOTE: The same area will be used by OEMuROT 
 *       If swap mode is used, make sure the layout definition 
 *       for scratch slot is consistant between OEMiROT and
 *       OEMuROT. 
 *       i.e. Scratch slot is after OEMiROT (and loader if used)
 *------------------------------------------------------------
 */
#if defined(FLASH_AREA_SCRATCH_ID)
#define FLASH_AREA_SCRATCH_DEVICE_ID    (FLASH_DEVICE_ID - FLASH_DEVICE_ID)
#define FLASH_AREA_SCRATCH_OFFSET       (FLASH_AREA_BL2_OFFSET + FLASH_AREA_BL2_SIZE)
#if defined(MCUBOOT_OVERWRITE_ONLY)
#define FLASH_AREA_SCRATCH_SIZE         (0x0000) /* Not used in MCUBOOT_OVERWRITE_ONLY mode */
#else
#define FLASH_AREA_SCRATCH_SIZE         (0x10000) /* 64 KB */
#endif
/* control scratch area */
#if (FLASH_AREA_SCRATCH_OFFSET % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0
#error "FLASH_AREA_SCRATCH_OFFSET not aligned on FLASH_AREA_IMAGE_SECTOR_SIZE"
#endif /* (FLASH_AREA_SCRATCH_OFFSET % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0*/
#else /* FLASH_AREA_SCRATCH_ID */
#define FLASH_AREA_SCRATCH_SIZE         (0x0)
#endif /* FLASH_AREA_SCRATCH_ID */


/* BL2 partitions size */
/*----------------------------------------
 * 4. OEMuROT Code area (primary slot)
 *----------------------------------------
 */
#define FLASH_S_PARTITION_SIZE          (0x18000) /* 72KB for OEMuROT */
#define FLASH_NS_PARTITION_SIZE         (0) /* No NS part for OEMuROT */
#define FLASH_PARTITION_SIZE            (FLASH_S_PARTITION_SIZE+FLASH_NS_PARTITION_SIZE)

#define OEMuROT_PARTITION_SIZE    FLASH_PARTITION_SIZE

#define FLASH_MAX_PARTITION_SIZE   OEMuROT_PARTITION_SIZE

/* BL2 flash areas */
#define FLASH_AREA_BEGIN_OFFSET         (FLASH_AREA_BL2_OFFSET + \
                                         FLASH_AREA_SCRATCH_SIZE + \
                                         FLASH_AREA_BL2_SIZE)
#define FLASH_AREAS_DEVICE_ID           (FLASH_DEVICE_ID - FLASH_DEVICE_ID)

/* Secure app (OEMuROT) image primary slot */
#define FLASH_AREA_0_DEVICE_ID          (FLASH_AREAS_DEVICE_ID)
#define FLASH_AREA_0_OFFSET             (FLASH_AREA_BEGIN_OFFSET)
#define FLASH_AREA_0_SIZE               (FLASH_PARTITION_SIZE)

/* Control Secure app image primary slot */
#if (FLASH_AREA_0_OFFSET  % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0
#error "FLASH_AREA_0_OFFSET  not aligned on FLASH_AREA_IMAGE_SECTOR_SIZE"
#endif /*  (FLASH_AREA_0_OFFSET  % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0 */

/*------------------------------------------
 * 5. OEMuROT download area (secondary slot)
 *------------------------------------------
 */
/* Secure app (OEMuROT) image secondary slot */
#define FLASH_AREA_2_DEVICE_ID          (FLASH_AREAS_DEVICE_ID)
#define FLASH_AREA_2_OFFSET             (FLASH_AREA_BEGIN_OFFSET + FLASH_AREA_0_SIZE)
#define FLASH_AREA_2_SIZE               (FLASH_PARTITION_SIZE)

/* Control Secure app image secondary slot */
#if (FLASH_AREA_2_OFFSET  % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0
#error "FLASH_AREA_2_OFFSET  not aligned on FLASH_AREA_IMAGE_SECTOR_SIZE"
#endif /*   (FLASH_AREA_2_OFFSET  % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0 */

/* Control flash area end */
#if (FLASH_AREA_END_OFFSET  % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0
#error "FLASH_AREA_END_OFFSET  not aligned on FLASH_AREA_IMAGE_SECTOR_SIZE"
#endif /*  (FLASH_AREA_END_OFFSET  % FLASH_AREA_IMAGE_SECTOR_SIZE) != 0 */

/*------------------------------------------------------------
 * Now reach the end of flash area known by OEMiROT 
 * The rest part of the flash is no more concerned by OEMiROT
 *------------------------------------------------------------
 */
/* flash areas end offset */
#define FLASH_AREA_END_OFFSET           (FLASH_AREA_BEGIN_OFFSET + FLASH_AREA_0_SIZE + \
                                         FLASH_AREA_2_SIZE)
/*
 * The maximum number of status entries supported by the bootloader.
 */
#if defined(MCUBOOT_OVERWRITE_ONLY)
#define MCUBOOT_STATUS_MAX_ENTRIES        (0)
#else /* not MCUBOOT_OVERWRITE_ONLY */
#define MCUBOOT_STATUS_MAX_ENTRIES        (((FLASH_MAX_PARTITION_SIZE - 1) / \
                                            FLASH_AREA_SCRATCH_SIZE) + 1)
#endif /* MCUBOOT_OVERWRITE_ONLY */
/* Maximum number of image sectors supported by the bootloader. */
#define MCUBOOT_MAX_IMG_SECTORS           ((FLASH_MAX_PARTITION_SIZE) / \
                                           FLASH_AREA_IMAGE_SECTOR_SIZE)

#define SECURE_IMAGE_OFFSET             (0x0)
#define SECURE_IMAGE_MAX_SIZE           FLASH_S_PARTITION_SIZE

#define NON_SECURE_IMAGE_OFFSET         (SECURE_IMAGE_OFFSET + SECURE_IMAGE_MAX_SIZE)
#define NON_SECURE_IMAGE_MAX_SIZE       FLASH_NS_PARTITION_SIZE


/* Flash device name used by BL2 and NV Counter
 * Name is defined in flash driver file: low_level_flash.c
 */
#define FLASH_DEV_NAME                             Driver_FLASH0

/* OBK */
#define OBK_HDPL0_OFFSET        (0x00U)         /* First OBkey Hdpl 0 */
#define OBK_HDPL0_END           (0xFFU)         /* Last OBKey Hdpl 0 */
#define OBK_HDPL1_OFFSET        (0x100U)        /* First OBkey Hdpl 1 */
#define OBK_HDPL1_END           (0x8FFU)        /* Last OBKey Hdpl 1 */
#define OBK_HDPL2_OFFSET        (0x900U)        /* First OBkey Hdpl 2 */
#define OBK_HDPL2_END           (0xBFFU)        /* Last OBKey Hdpl 2 */
#define OBK_HDPL3_OFFSET        (0xC00U)        /* First OBkey Hdpl 3 */
#define OBK_HDPL3_END           (0x1FFFU)       /* Last OBKey Hdpl 3 */

/* Engi bits */
#define ENGI_BASE_NS                        (0x08FFF800U)
#define ENGI_SIZE                           (0x40U)

/* Systeme Flash description */
#define RSS_LIB_BASE                        (0x0FF94000U)
#define RSS_LIB_SIZE                        (0x2000U)
#define BOOTLOADER_BASE_NS                  (0x0BF97000U)
#define BOOTLOADER_SIZE                     (0x9400U)
#define STM32_DESCRIPTOR_BASE_NS_3          (0x0BF9FB00U)
#define STM32_DESCRIPTOR_BASE_NS_2          (0x0BF9FD00U)
#define STM32_DESCRIPTOR_BASE_NS_1          (0x0BF9FE00U)
#define RSSLIB_PFUNC_3                      (0x0BF9FB68UL)
#define RSSLIB_PFUNC_2                      (0x0BF9FD68UL)
#define RSSLIB_PFUNC_1                      (0x0BF9FE68UL)
#define STM32_DESCRIPTOR_SIZE               (0x100U)
#define STM32_DESCRIPTOR_BASE_NS            (STM32_DESCRIPTOR_BASE_NS_3) /* use for mpu region the lowest address*/
#define STM32_DESCRIPTOR_END_NS             (STM32_DESCRIPTOR_BASE_NS_1 + STM32_DESCRIPTOR_SIZE -1) /* to cover all descriptors */

#endif /* __FLASH_LAYOUT_H__ */
