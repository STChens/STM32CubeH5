/**
  ******************************************************************************
  * @file    common.c
  * @author  MCD Application Team
  * @brief   COMMON module.
  *          This file provides set of firmware functions to manage Common
  *          functionalities.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/** @addtogroup USER_APP User App Example
  * @{
  */
/** @addtogroup USER_APP_COMMON Common
  * @{
  */
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "com.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Convert a string to an integer
  * @param  pInputStr: The string to be converted
  * @param  pIntNum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
uint32_t Str2Int(uint8_t *pInputStr, uint32_t *pIntNum)
{
  uint32_t i = 0U;
  uint32_t res = 0U;
  uint32_t val = 0U;

  if ((pInputStr[0U] == '0') && ((pInputStr[1U] == 'x') || (pInputStr[1U] == 'X')))
  {
    i = 2U;
    while ((i < 11U) && (pInputStr[i] != '\0'))
    {
      if (ISVALIDHEX(pInputStr[i]))
      {
        val = (val << 4U) + CONVERTHEX(pInputStr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0U;
        break;
      }
      i++;
    }

    /* valid result */
    if (pInputStr[i] == '\0')
    {
      *pIntNum = val;
      res = 1U;
    }
  }
  else /* max 10-digit decimal input */
  {
    while ((i < 11U) && (res != 1U))
    {
      if (pInputStr[i] == '\0')
      {
        *pIntNum = val;
        /* return 1 */
        res = 1U;
      }
      else if (((pInputStr[i] == 'k') || (pInputStr[i] == 'K')) && (i > 0U))
      {
        val = val << 10U;
        *pIntNum = val;
        res = 1U;
      }
      else if (((pInputStr[i] == 'm') || (pInputStr[i] == 'M')) && (i > 0U))
      {
        val = val << 20U;
        *pIntNum = val;
        res = 1U;
      }
      else if (ISVALIDDEC(pInputStr[i]))
      {
        val = val * 10U + CONVERTDEC(pInputStr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0U;
        break;
      }
      i++;
    }
  }

  return res;
}

/**
  * @brief  Convert an integer to a hex string 
  * @param  pOutStr: The buffer to store the converted string
  * @param  IntNum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
int Int2HexStr(uint8_t *pOutStr, uint32_t IntNum)
{
  int len=0;
  int hex=8;
  if ( pOutStr != NULL )
  {
    if ( IntNum == 0 )
    {
        pOutStr[0] = '0';
        pOutStr[1] = '\0';
        len=2;
    }
    else
    {
      while(hex>0)
      {
        pOutStr[len]=(IntNum&0xF0000000)>>28;
        /* we start from the first non-zero number*/
        if(pOutStr[len]>0 || len > 0) 
        {
          pOutStr[len] = pOutStr[len]>9 ? 'A'+pOutStr[len]-0xA : '0'+pOutStr[len];
          len++;        
        }
        hex--;
        IntNum<<=4;
      }
      pOutStr[len] = '\0';
      len++;
    }
  }
  
  return len;
}

/**
  * @brief  Convert an integer to a decimal string 
  * @param  pOutStr: The buffer to store the converted string
  * @param  IntNum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
int Int2DecStr(uint8_t *pOutStr, uint32_t IntNum)
{
  int len=0;
  int dec=10;
  char intstr[12] = {'\0'};
  
  if ( pOutStr != NULL )
  {
    if ( IntNum == 0 )
    {
        pOutStr[0] = '0';
        pOutStr[1] = '\0';
        len=2;
    }
    else
    {
      while(dec>=0)
      {
        intstr[dec] = IntNum%10;
        IntNum-=intstr[dec];
        IntNum/=10;
        intstr[dec] = intstr[dec]+'0';
        dec--;        
      }
      len=0;
      for (dec =0;dec<12;dec++)
      {
        /* we copy from the first non-zero number */
        if(intstr[dec]!='0' || len>0)
        {
          pOutStr[len] = intstr[dec];
          len++;
        }
      }
    }
  }
  
  return len;
}

/**
  * @brief  Transmit a byte to the HyperTerminal
  * @param  param The byte to be sent
  * @retval HAL_StatusTypeDef HAL_OK if OK
  */
HAL_StatusTypeDef Serial_PutByte(uint8_t uParam)
{
  return COM_Transmit_Y(&uParam, 1U, TX_TIMEOUT);
}
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
