/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    test_ecdsa.c
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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Extract from SigGen.txt
  * [P-256,SHA-224]

Msg = ff624d0ba02c7b6370c1622eec3fa2186ea681d1659e0a845448e777b75a8e77a77bb26e5733179d58ef9bc8a4e8b69
71aef2539f77ab0963a3415bbd6258339bd1bf55de65db520c63f5b8eab3d55debd05e9494212170f5d65b3286b8b668705b1
e2b2b5568610617abb51d2dd0cb450ef59df4b907da90cfa7b268de8c4c2
d = 708309a7449e156b0db70e5b52e606c7e094ed676ce8953bf6c14757c826f590
Qx = 29578c7ab6ce0d11493c95d5ea05d299d536801ca9cbd50e9924e43b733b83ab
Qy = 08c8049879c6278b2273348474158515accaa38344106ef96803c5a05adc4800
k = 58f741771620bdc428e91a32d86d230873e9140336fcfb1e122892ee1d501bdc
R = 4a19274429e40522234b8785dc25fc524f179dcc95ff09b3c9770fc71f54ca0d
S = 58982b79a65b7320f5b92d13bdaecdd1259e760f0f718ba933fd098f6f75d4b7

  */
const uint8_t Message[] =
{
  0xff, 0x62, 0x4d, 0x0b, 0xa0, 0x2c, 0x7b, 0x63, 0x70, 0xc1, 0x62, 0x2e, 0xec, 0x3f, 0xa2, 0x18,
  0x6e, 0xa6, 0x81, 0xd1, 0x65, 0x9e, 0x0a, 0x84, 0x54, 0x48, 0xe7, 0x77, 0xb7, 0x5a, 0x8e, 0x77,
  0xa7, 0x7b, 0xb2, 0x6e, 0x57, 0x33, 0x17, 0x9d, 0x58, 0xef, 0x9b, 0xc8, 0xa4, 0xe8, 0xb6, 0x97,
  0x1a, 0xef, 0x25, 0x39, 0xf7, 0x7a, 0xb0, 0x96, 0x3a, 0x34, 0x15, 0xbb, 0xd6, 0x25, 0x83, 0x39,
  0xbd, 0x1b, 0xf5, 0x5d, 0xe6, 0x5d, 0xb5, 0x20, 0xc6, 0x3f, 0x5b, 0x8e, 0xab, 0x3d, 0x55, 0xde,
  0xbd, 0x05, 0xe9, 0x49, 0x42, 0x12, 0x17, 0x0f, 0x5d, 0x65, 0xb3, 0x28, 0x6b, 0x8b, 0x66, 0x87,
  0x05, 0xb1, 0xe2, 0xb2, 0xb5, 0x56, 0x86, 0x10, 0x61, 0x7a, 0xbb, 0x51, 0xd2, 0xdd, 0x0c, 0xb4,
  0x50, 0xef, 0x59, 0xdf, 0x4b, 0x90, 0x7d, 0xa9, 0x0c, 0xfa, 0x7b, 0x26, 0x8d, 0xe8, 0xc4, 0xc2
};
const uint8_t Private_Key[] =
{
  0x70, 0x83, 0x09, 0xa7, 0x44, 0x9e, 0x15, 0x6b, 0x0d, 0xb7, 0x0e, 0x5b, 0x52, 0xe6, 0x06, 0xc7,
  0xe0, 0x94, 0xed, 0x67, 0x6c, 0xe8, 0x95, 0x3b, 0xf6, 0xc1, 0x47, 0x57, 0xc8, 0x26, 0xf5, 0x90
};
const uint8_t Public_Key[] =
{
  0x04, /* This octet is for a Weierstrass public key representation */
  0x29, 0x57, 0x8c, 0x7a, 0xb6, 0xce, 0x0d, 0x11, 0x49, 0x3c, 0x95, 0xd5, 0xea, 0x05, 0xd2, 0x99,
  0xd5, 0x36, 0x80, 0x1c, 0xa9, 0xcb, 0xd5, 0x0e, 0x99, 0x24, 0xe4, 0x3b, 0x73, 0x3b, 0x83, 0xab,
  0x08, 0xc8, 0x04, 0x98, 0x79, 0xc6, 0x27, 0x8b, 0x22, 0x73, 0x34, 0x84, 0x74, 0x15, 0x85, 0x15,
  0xac, 0xca, 0xa3, 0x83, 0x44, 0x10, 0x6e, 0xf9, 0x68, 0x03, 0xc5, 0xa0, 0x5a, 0xdc, 0x48, 0x00
};

const uint8_t Known_Signature[] =
{
  0x24, 0xe0, 0x43, 0x67, 0xe6, 0x01, 0x42, 0xf0, 0xa4, 0xd4, 0xa4, 0xbd, 0x19, 0x38, 0x3a, 0xc8,
  0x82, 0xe4, 0x4a, 0xd2, 0x99, 0x57, 0xe7, 0x67, 0x77, 0x0b, 0xdf, 0xa1, 0xd8, 0xf0, 0xf7, 0x9c,
  0xed, 0x38, 0x82, 0xf1, 0xc6, 0x37, 0xd8, 0x26, 0x9b, 0xb6, 0xa1, 0xe7, 0x2d, 0x6b, 0x9d, 0x61,
  0xb8, 0x89, 0xfc, 0x24, 0x84, 0x3c, 0x51, 0xe3, 0xec, 0xe7, 0xe7, 0xd2, 0x9d, 0x28, 0x36, 0xa4
};

/* Computed data buffer */
uint8_t Computed_Hash[28]; /* SHA224 length */
uint8_t Computed_Signature[sizeof(Known_Signature)];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
void test_ecdsa(void)
{
  ENABLE_DWT_COUNTER();
  
  /* USER CODE BEGIN 1 */
  psa_status_t retval;
  size_t computed_size;
  /* Key attributes */
  psa_key_attributes_t key_attributes;
  psa_key_handle_t key_handle_public;       /* Public key */
  psa_key_handle_t key_handle_private_det;  /* Private deterministic key */
  psa_key_handle_t key_handle_private;      /* Private key */

  printf("** Test ECDSA ALT ** \n\r");
  /* --------------------------------------------------------------------------
   * Create the PSA key
   * --------------------------------------------------------------------------
  */

  /* Init the key attributes */
  key_attributes = psa_key_attributes_init();

  /* Init the PSA */
  printf("Call psa_aead_encrypt\r\n");
  retval = psa_crypto_init();
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }
  /* Setup the key policy for public key */
  psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
  psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_224));
  psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));

  /* Import a public key */
  printf("Call psa_import_key to import public key\r\n");
  retval = psa_import_key(&key_attributes, Public_Key, sizeof(Public_Key), &key_handle_public);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Reset the key attribute */
  printf("Call psa_import_key to import private key\r\n");
  psa_reset_key_attributes(&key_attributes);

  /* Setup the key policy for deterministic private key */
  printf("Setup the key policy for deterministic private key\r\n");
  psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
  psa_set_key_algorithm(&key_attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_224));
  psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
  psa_set_key_bits(&key_attributes, 8U*sizeof(Private_Key));

  /* Import a private key */
  printf("Call psa_import_key to import private key\r\n");
  retval = psa_import_key(&key_attributes, Private_Key, sizeof(Private_Key), &key_handle_private_det);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Reset the key attribute */
  printf("Call psa_reset_key_attributes\r\n");
  psa_reset_key_attributes(&key_attributes);

  /* Setup the key policy for private key */
  printf("Setup the key policy for private key\r\n");
  psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
  psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_224));
  psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
  psa_set_key_bits(&key_attributes, 8U*sizeof(Private_Key));

  /* Import a private key */
  printf("Call psa_import_key to import private key\r\n");
  retval = psa_import_key(&key_attributes, Private_Key, sizeof(Private_Key), &key_handle_private);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Reset the key attribute */
  printf("Call psa_reset_key_attributes\r\n");
  psa_reset_key_attributes(&key_attributes);

  /* --------------------------------------------------------------------------
   * Compute the digest
   * --------------------------------------------------------------------------
  */

  /* Compute directly the digest passing all the needed parameters */
  printf("Call psa_hash_compute\r\n");
  retval = psa_hash_compute(PSA_ALG_SHA_224,
                            Message, sizeof(Message), /* Message to digest */
                            Computed_Hash,            /* Data buffer to receive digest data */
                            sizeof(Computed_Hash),    /* Size of hash buffer */
                            &computed_size);          /* Size of computed digest */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Verify generated data size is the expected one */
  printf("Check computed size\r\n");
  if (computed_size != sizeof(Computed_Hash))
  {
    printf("computed_size = %d\r\n", computed_size);
    Error_Handler();
  }

  /* --------------------------------------------------------------------------
   * KNOWN RANDOM USAGE
   * --------------------------------------------------------------------------
   */

  /* Compute directly the signature passing all the needed parameters */
  printf("Call psa_sign_hash\r\n");
  retval = psa_sign_hash(key_handle_private_det,
                         PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_224),   /* Algorithm type */
                         Computed_Hash, 28,                              /* Digest to sign */
                         Computed_Signature, sizeof(Computed_Signature), /*Data buf to receive signature*/
                         &computed_size);                                /* Size of computed signature */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Verify generated data size is the expected one */
  printf("Check computed size\r\n");
  if (computed_size != sizeof(Known_Signature))
  {
    printf("computed_size = %d\r\n", computed_size);
    Error_Handler();
  }

  /* Verify generated data are the expected ones */
  printf("Verify generated data are the expected ones\r\n");
  if (memcmp(Computed_Signature, Known_Signature, computed_size) != 0)
  {
    printf("Computed_Signature not equal to Known_Signature\r\n");
    Error_Handler();
  }

  /* Verify directly the signature passing all the needed parameters */
  printf("Call psa_verify_hash\r\n");
  retval = psa_verify_hash(key_handle_public,
                           PSA_ALG_ECDSA(PSA_ALG_SHA_224),            /* Algorithm type */
                           Computed_Hash, sizeof(Computed_Hash),      /* Digest to verify */
                           Known_Signature, sizeof(Known_Signature)); /* Signature to verify */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* --------------------------------------------------------------------------
   * TRUE RANDOM USAGE
   * --------------------------------------------------------------------------
   */

  /* Compute directly the signature passing all the needed parameters */
  printf("Call psa_sign_hash\r\n");
  retval = psa_sign_hash(key_handle_private,
                         PSA_ALG_ECDSA(PSA_ALG_SHA_224),                 /* Algorithm type */
                         Computed_Hash, sizeof(Computed_Hash),           /* Digest to sign */
                         Computed_Signature, sizeof(Computed_Signature), /*Data buf to receive signature*/
                         &computed_size);                                /* Size of computed signature */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Verify generated data size is the expected one */
  printf("Check computed size\r\n");
  if (computed_size != sizeof(Known_Signature))
  {
    printf("computed_size = %d\r\n", computed_size);
    Error_Handler();
  }

  /* Verify directly the signature passing all the needed parameters */
  printf("Call psa_verify_hash\r\n");
  retval = psa_verify_hash(key_handle_public,
                           PSA_ALG_ECDSA(PSA_ALG_SHA_224),                  /* Algorithm type */
                           Computed_Hash, sizeof(Computed_Hash),            /* Digest to verify */
                           Computed_Signature, sizeof(Computed_Signature)); /* Signature tu verify */

  /* Verify API returned value */
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* --------------------------------------------------------------------------
   * Destroy the PSA keys and clear all data
   * --------------------------------------------------------------------------
  */
  /* Destroy the public key */
  printf("Call psa_destroy_key (public key)\r\n");
  retval = psa_destroy_key(key_handle_public);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Destroy the private key */
  printf("Call psa_destroy_key (private key)\r\n");
  retval = psa_destroy_key(key_handle_private);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Destroy the private deterministic key */
  printf("Call psa_destroy_key (private deterministic key)\r\n");  
  retval = psa_destroy_key(key_handle_private_det);
  if (retval != PSA_SUCCESS)
  {
    printf("retval = %x\r\n", retval);
    Error_Handler();
  }

  /* Clear all data associated with the PSA layer */
  mbedtls_psa_crypto_free();

  /* Turn on LED3 in an infinite loop in case of ECC ECDSA operations are successful */
  BSP_LED_On(LED3);
  printf("Test PASSED\r\n");

  return;
}

