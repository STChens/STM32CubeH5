/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    test_aes_gcm.c
  * @author  MCD Application Team
  * @brief   This example shows how to retarget the C library printf function
  *          to the UART.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "crypto.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  FAILED = 0,
  PASSED = 1
} TestStatus;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CHUNK_SIZE  48u

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/** Extract from NIST Special Publication 800-38D
  * gcmEncryptExtIV256.rsp
[Keylen = 128]
[IVlen = 96]
[PTlen = 408]
[AADlen = 384]
[Taglen = 128]

Count = 2
Key = 463b412911767d57a0b33969e674ffe7845d313b88c6fe312f3d724be68e1fca
IV = 611ce6f9a6880750de7da6cb
PT = e7d1dcf668e2876861940e012fe52a98dacbd78ab63c08842cc9801ea581682ad54af0c34d0d7f6f59e8ee0bf4900e0fd85042
AAD = 0a682fbc6192e1b47a5e0868787ffdafe5a50cead3575849990cdd2ea9b3597749403efb4a56684f0c6bde352d4aeec5
CT = 8886e196010cb3849d9c1a182abe1eeab0a5f3ca423c3669a4a8703c0f146e8e956fb122e0d721b869d2b6fcd4216d7d4d3758
Tag = 2469cecd70fd98fec9264f71df1aee9a
  */
const uint8_t Key[] =
{
  0x46, 0x3b, 0x41, 0x29, 0x11, 0x76, 0x7d, 0x57, 0xa0, 0xb3, 0x39, 0x69, 0xe6, 0x74, 0xff, 0xe7,
  0x84, 0x5d, 0x31, 0x3b, 0x88, 0xc6, 0xfe, 0x31, 0x2f, 0x3d, 0x72, 0x4b, 0xe6, 0x8e, 0x1f, 0xca
};
const uint8_t IV[] =
{
  0x61, 0x1c, 0xe6, 0xf9, 0xa6, 0x88, 0x07, 0x50, 0xde, 0x7d, 0xa6, 0xcb
};
const uint8_t Plaintext[] =
{
  0xe7, 0xd1, 0xdc, 0xf6, 0x68, 0xe2, 0x87, 0x68, 0x61, 0x94, 0x0e, 0x01, 0x2f, 0xe5, 0x2a, 0x98,
  0xda, 0xcb, 0xd7, 0x8a, 0xb6, 0x3c, 0x08, 0x84, 0x2c, 0xc9, 0x80, 0x1e, 0xa5, 0x81, 0x68, 0x2a,
  0xd5, 0x4a, 0xf0, 0xc3, 0x4d, 0x0d, 0x7f, 0x6f, 0x59, 0xe8, 0xee, 0x0b, 0xf4, 0x90, 0x0e, 0x0f,
  0xd8, 0x50, 0x42
};
const uint8_t AddData[] =
{
  0x0a, 0x68, 0x2f, 0xbc, 0x61, 0x92, 0xe1, 0xb4, 0x7a, 0x5e, 0x08, 0x68, 0x78, 0x7f, 0xfd, 0xaf,
  0xe5, 0xa5, 0x0c, 0xea, 0xd3, 0x57, 0x58, 0x49, 0x99, 0x0c, 0xdd, 0x2e, 0xa9, 0xb3, 0x59, 0x77,
  0x49, 0x40, 0x3e, 0xfb, 0x4a, 0x56, 0x68, 0x4f, 0x0c, 0x6b, 0xde, 0x35, 0x2d, 0x4a, 0xee, 0xc5
};
const uint8_t Expected_Ciphertext[] =
{
  /* Ciphertext */
  0x88, 0x86, 0xe1, 0x96, 0x01, 0x0c, 0xb3, 0x84, 0x9d, 0x9c, 0x1a, 0x18, 0x2a, 0xbe, 0x1e, 0xea,
  0xb0, 0xa5, 0xf3, 0xca, 0x42, 0x3c, 0x36, 0x69, 0xa4, 0xa8, 0x70, 0x3c, 0x0f, 0x14, 0x6e, 0x8e,
  0x95, 0x6f, 0xb1, 0x22, 0xe0, 0xd7, 0x21, 0xb8, 0x69, 0xd2, 0xb6, 0xfc, 0xd4, 0x21, 0x6d, 0x7d,
  0x4d, 0x37, 0x58,
};
const uint8_t Expected_Tag[] =
{
  0x24, 0x69, 0xce, 0xcd, 0x70, 0xfd, 0x98, 0xfe, 0xc9, 0x26, 0x4f, 0x71, 0xdf, 0x1a, 0xee, 0x9a
};

const uint8_t Expected_Ciphertext_and_Tag[] =
{
  0x88, 0x86, 0xe1, 0x96, 0x01, 0x0c, 0xb3, 0x84, 0x9d, 0x9c, 0x1a, 0x18, 0x2a, 0xbe, 0x1e, 0xea,
  0xb0, 0xa5, 0xf3, 0xca, 0x42, 0x3c, 0x36, 0x69, 0xa4, 0xa8, 0x70, 0x3c, 0x0f, 0x14, 0x6e, 0x8e,
  0x95, 0x6f, 0xb1, 0x22, 0xe0, 0xd7, 0x21, 0xb8, 0x69, 0xd2, 0xb6, 0xfc, 0xd4, 0x21, 0x6d, 0x7d,
  0x4d, 0x37, 0x58,
  0x24, 0x69, 0xce, 0xcd, 0x70, 0xfd, 0x98, 0xfe, 0xc9, 0x26, 0x4f, 0x71, 0xdf, 0x1a, 0xee, 0x9a
};

/* Computed data buffer */
uint8_t Computed_Ciphertext[sizeof(Expected_Ciphertext) + sizeof(Expected_Tag)];
uint8_t Computed_Plaintext[sizeof(Plaintext)];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
void test_aes_gcm(void)
{

  /* USER CODE BEGIN 1 */
  psa_status_t retval;
  size_t computed_size;
  /* Key attributes */
  psa_key_attributes_t key_attributes;
  psa_key_handle_t key_handle;

  printf("** Test AES GCM ALT ** \n\r");
  /* --------------------------------------------------------------------------
   * Create the PSA key
   * --------------------------------------------------------------------------
  */

  /* Init the key attributes */
  (void) memset(&key_attributes, 0, sizeof(key_attributes));

  /* Init the PSA */
  printf("Call psa_crypto_init\r\n");
  retval = psa_crypto_init();
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Setup the key policy */
  printf("Setup the key policy\r\n");
  psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
  psa_set_key_algorithm(&key_attributes, PSA_ALG_GCM);
  psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
  psa_set_key_bits(&key_attributes, 8U*sizeof(Key));

  /* Import a key */
  printf("Call psa_import_key\r\n");
  retval = psa_import_key(&key_attributes, Key, sizeof(Key), &key_handle);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Reset the key attribute */
  printf("Call psa_reset_key_attributes\r\n");
  psa_reset_key_attributes(&key_attributes);

  /* --------------------------------------------------------------------------
   * SINGLE CALL USAGE
   * --------------------------------------------------------------------------
   */

  printf("Call psa_aead_encrypt\r\n");
  /* Compute directly the ciphertext passing all the needed parameters */
  retval = psa_aead_encrypt(key_handle,                         /* The key id */
                            PSA_ALG_GCM,                        /* Algorithm type */
                            IV, sizeof(IV),                     /* Initialization vector */
                            AddData, sizeof(AddData),           /* Additional authenticated data */
                            Plaintext, sizeof(Plaintext),       /* Plaintext to encrypt and authenticate */
                            Computed_Ciphertext,                /* Data buffer to receive ciphertext and auth tag */
                            sizeof(Computed_Ciphertext),        /* Size of buffer to receive ciphertext and tag */
                            &computed_size);                    /* Size of computed ciphertext */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Verify generated data size is the expected one */
  printf("Check computed size\r\n");
  if (computed_size != (sizeof(Expected_Ciphertext) + sizeof(Expected_Tag)))
  {
    printf("computed_size = %x\r\n", computed_size);
    Error_Handler();
  }

  /* Verify generated data are the expected ones */
  printf("Verify generated data are the expected ones\r\n");
  if (memcmp(Expected_Ciphertext, Computed_Ciphertext, sizeof(Expected_Ciphertext)) != 0)
  {
    printf("Computed_Ciphertext not equal to Expected_Ciphertext\r\n");
    Error_Handler();
  }

  /* Verify generated authentication tag is the expected one */
  printf("Verify generated authentication tag is the expected one\r\n");
  if (memcmp(Expected_Tag, &Computed_Ciphertext[sizeof(Expected_Ciphertext)], sizeof(Expected_Tag)) != 0)
  {
    printf("Generated tag not equal to Expected_Tag\r\n");
    Error_Handler();
  }

  printf("Call psa_aead_decrypt\r\n");
  /* Decrypt and verify directly ciphertext and tag passing all the needed parameters */
  retval = psa_aead_decrypt(key_handle,                          /* The key id */
                            PSA_ALG_GCM,                         /* Algorithm type */
                            IV, sizeof(IV),                      /* Initialization vector */
                            AddData, sizeof(AddData),            /* Additional authenticated data */
                            Expected_Ciphertext_and_Tag,         /* Ciphertext + tag to decrypt and verify */
                            sizeof(Expected_Ciphertext_and_Tag), /* Sizeof Ciphertext + tag to decrypt and verify */
                            Computed_Plaintext,                  /* Data buffer to receive generated plaintext */
                            sizeof(Computed_Plaintext),          /* Size of data buff to receive generated plaintext */
                            &computed_size);                     /* Size of computed plaintext */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Verify generated data size is the expected one */
  printf("Check computed size\r\n");
  if (computed_size != sizeof(Plaintext))
  {
    printf("computed_size = %d\r\n", computed_size);
    Error_Handler();
  }

  /* Verify generated data are the expected ones */
  printf("Verify generated data are the expected ones\r\n");
  if (memcmp(Plaintext, Computed_Plaintext,
             computed_size) != 0)
  {
    printf("Plaintext not equal to Computed_Plaintext\r\n");
    Error_Handler();
  }

  /* --------------------------------------------------------------------------
   * Destroy the PSA key and clear all data
   * --------------------------------------------------------------------------
  */
  /* Destroy the key */
  printf("Call psa_destroy_key\r\n");
  retval = psa_destroy_key(key_handle);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Clear all data associated with the PSA layer */
  mbedtls_psa_crypto_free();

  /* Turn on LED3 in an infinite loop in case of AES GCM operations are successful */
  BSP_LED_On(LED3);
  printf("Test PASSED\r\n");

  return;
}

