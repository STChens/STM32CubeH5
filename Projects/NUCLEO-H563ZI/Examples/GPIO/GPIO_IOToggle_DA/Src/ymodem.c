/**
  ******************************************************************************
  * @file    ymodem.c
  * @author  MCD Application Team
  * @brief   Ymodem module.
  *          This file provides set of firmware functions to manage Ymodem
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

/** @addtogroup  FW_UPDATE Firmware Update Example
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

#include "com.h"
#include "common.h"
#include "ymodem.h"
#include "string.h"
#include "main.h"

/* Private const -------------------------------------------------------------*/
const char BACK_SLASH_POINT[]="\b.";
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* @note ATTENTION - please keep this variable 32bit aligned */
static uint8_t m_aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX +
                             PACKET_TRAILER_SIZE]; /*!<Array used to store Packet Data*/
static uint8_t m_aReadData[PACKET_1K_SIZE]; /*!<Array used to store Packet Data*/
uint8_t m_aFileName[FILE_NAME_LENGTH + 1U]; /*!< Array used to store File Name data */
static CRC_HandleTypeDef CrcHandle; /*!<CRC handle*/

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef ReceivePacket(uint8_t *pData, uint32_t *puLength, uint32_t uTimeout);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive a packet from sender
  * @param  pData
  * @param  puLength
  *     0: end of transmission
  *     2: abort by sender
  *    >0: packet length
  * @param  uTimeout
  * @retval HAL_OK: normally return
  *         HAL_BUSY: abort by user
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *pData, uint32_t *puLength, uint32_t uTimeout)
{
  uint32_t crc;
  uint32_t packet_size = 0U;
  HAL_StatusTypeDef status;
  uint8_t char1;
  uint8_t char2;
  uint8_t char3;

  *puLength = 0U;

  /* If the SecureBoot configured the IWDG, UserApp must reload IWDG counter with
   *value defined in the reload register */
  status = (HAL_StatusTypeDef)COM_Receive_Y(&char1, 1, uTimeout);

  if (status == HAL_OK)
  {
    switch (char1)
    {
      case SOH:
        packet_size = PACKET_SIZE;
        break;
      case STX:
        packet_size = PACKET_1K_SIZE;
        break;
      case EOT:
        packet_size = 0;
        break;
      case CA:
        if ((COM_Receive_Y(&char1, 1U, uTimeout) == HAL_OK) && (char1 == CA))
        {
          packet_size = 2U;
        }
        else
        {
          status = HAL_ERROR;
        }
        break;
      case ABORT1:
      case ABORT2:
        status = HAL_BUSY;
        break;
      case RB:
        if ((COM_Receive_Y(&char2, 1U, uTimeout) == HAL_OK) &&                /* Ymodem startup sequence : rb ==> 0x72 + 0x62 + 0x0D */
            (COM_Receive_Y(&char3, 1U, uTimeout) == HAL_OK) &&
            (char2 == 0x62) &&
            (char3 == 0xd))
            {
                packet_size = 3U;
                break;
            }
      default:
        status = HAL_ERROR;
        break;
    }
    *pData = char1;

    if (packet_size >= PACKET_SIZE)
    {
      status = COM_Receive_Y(&pData[PACKET_NUMBER_INDEX], packet_size + PACKET_OVERHEAD_SIZE, uTimeout);

      /* Simple packet sanity check */
      if (status == HAL_OK)
      {
        if (pData[PACKET_NUMBER_INDEX] != ((pData[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE))
        {
          packet_size = 0U;
          status = HAL_ERROR;
        }
        else
        {
          /* Check packet CRC */
          crc = pData[ packet_size + PACKET_DATA_INDEX ] << 8U;
          crc += pData[ packet_size + PACKET_DATA_INDEX + 1U ];
          if (HAL_CRC_Calculate(&CrcHandle, (uint32_t *)&pData[PACKET_DATA_INDEX], packet_size) != crc)
          {
            packet_size = 0U;
            status = HAL_ERROR;
          }
        }
      }
      else
      {
        packet_size = 0U;
      }
    }
  }
  *puLength = packet_size;
  return status;
}


/**
  * @brief  Init of Ymodem module.
  * @param None.
  * @retval None.
  */
void Ymodem_Init(void)
{
  __HAL_RCC_CRC_CLK_ENABLE();

  /*-1- Configure the CRC peripheral */
  CrcHandle.Instance = CRC;

  /* The CRC-16-CCIT polynomial is used */
  CrcHandle.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_DISABLE;
  CrcHandle.Init.GeneratingPolynomial    = 0x1021U;
  CrcHandle.Init.CRCLength               = CRC_POLYLENGTH_16B;

  /* The zero init value is used */
  CrcHandle.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_DISABLE;
  CrcHandle.Init.InitValue               = 0U;

  /* The input data are not inverted */
  CrcHandle.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;

  /* The output data are not inverted */
  CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;

  /* The input data are 32-bit long words */
  CrcHandle.InputDataFormat              = CRC_INPUTDATA_FORMAT_BYTES;

  if (HAL_CRC_Init(&CrcHandle) != HAL_OK)
  {
    /* Initialization Error */
    while (1);
  }
  if (COM_Y_On(CA) != HAL_OK)
  {
    while(1);
  }
}

/**
  * @brief  Receive a file using the ymodem protocol with CRC16.
  * @param  puSize The uSize of the file.
  * @param  uMemDestination where the file has to be downloaded.
  * @retval COM_StatusTypeDef result of reception/programming
  */
COM_StatusTypeDef Ymodem_Receive(uint32_t *puSize, uint32_t uMemDestination,
                                 Ymodem_HeaderPktCB_t hcb, 
                                 Ymodem_DataPktCB_t dcb)
{
  uint32_t i, packet_length, session_done = 0U, file_done, errors = 0U, session_begin = 0U;
  uint32_t eot = 0U;
  uint32_t ramsource, filesize;
  uint8_t *file_ptr;
  uint8_t file_size[FILE_SIZE_LENGTH + 1U], tmp;
  uint32_t packets_received;
  COM_StatusTypeDef e_result = COM_OK;
  uint32_t cause = 0;
  while ((session_done == 0U) && (e_result == COM_OK))
  {
    packets_received = 0U;
    file_done = 0U;
    while ((file_done == 0U) && (e_result == COM_OK))
    {
      switch (ReceivePacket(m_aPacketData, &packet_length, DOWNLOAD_TIMEOUT))
      {
        case HAL_OK:
          errors = 0U;
          switch (packet_length)
          {
            case 3U:
              /* Startup sequence */
              break;
            case 2U:
              /* Abort by sender */
              Serial_PutByte(ACK);
              e_result = COM_ABORT;
              break;
            case 0U:
              /* End of transmission */
              if (!eot)
              {
                    Serial_PutByte(NAK);
                    eot = 1;
              }
              else
              {
                    Serial_PutByte(ACK);
                    *puSize = filesize;
                    file_done = 1U;
              }
              break;
            default:
              /* Normal packet */
              if (m_aPacketData[PACKET_NUMBER_INDEX] != (packets_received & 0xff))
              {
                /*             Serial_PutByte(NAK);*/
              }
              else
              {
                if (packets_received == 0U)
                {
                  /* File name packet */
                  if (m_aPacketData[PACKET_DATA_INDEX] != 0U)
                  {
                    /* File name extraction */
                    i = 0U;
                    file_ptr = m_aPacketData + PACKET_DATA_INDEX;
                    while ((*file_ptr != 0U) && (i < FILE_NAME_LENGTH))
                    {
                      m_aFileName[i++] = *file_ptr++;
                    }

                    /* File size extraction */
                    m_aFileName[i++] = '\0';
                    i = 0U;
                    file_ptr ++;
                    while ((*file_ptr != ' ') && (i < FILE_SIZE_LENGTH))
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    Str2Int(file_size, &filesize);
                    if ((uint32_t)filesize > *puSize)
                    {
                      *puSize = 0;
                      tmp = CA;
                      e_result = COM_ABORT;
                      cause = 1;
                    }

                    /* Header packet received callback call*/
                    if ((*puSize) && (hcb(uMemDestination, (uint32_t) filesize) == HAL_OK))
                    {

                      Serial_PutByte(ACK);
                      COM_Flush();
                      Serial_PutByte(CRC16);
                    }
                    else
                    {
                      /* End session */
                      COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                      COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                      cause = 2;
                      break;
                    }

                  }
                  /* File header packet is empty, end session */
                  else
                  {
                    Serial_PutByte(ACK);
                    file_done = 1;
                    session_done = 1;
                    cause = 3;
                    break;
                  }
                }
                else /* Data packet */
                {
                  ramsource = (uint32_t) & m_aPacketData[PACKET_DATA_INDEX];

                  /* Data packet received callback call*/
                  if ((*puSize) && (dcb((uint8_t *)ramsource, uMemDestination, (uint32_t) packet_length) == HAL_OK))
                  {
                    uMemDestination += (packet_length);
                    Serial_PutByte(ACK);
                  }
                  else /* An error occurred while writing to Flash memory */
                  {
                    /* End session */
                    COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                    COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                    cause = 4;
                  }
                }
                packets_received ++;
                session_begin = 1U;
              }
              break;
          }
          break;
        case HAL_BUSY: /* Abort actually */
          Serial_PutByte(CA);
          Serial_PutByte(CA);
          e_result = COM_ABORT;
          cause  = 5;
          break;
        default:
          if (session_begin > 0U)
          {
            errors ++;
          }
          if (errors > MAX_ERRORS)
          {
            /* Abort communication */
            Serial_PutByte(CA);
            Serial_PutByte(CA);
          }
          else
          {
            Serial_PutByte(CRC16); /* Ask for a packet */
            /* Replace C char by . on display console */
            COM_Transmit_Y((uint8_t *)BACK_SLASH_POINT, sizeof(BACK_SLASH_POINT)-1, TX_TIMEOUT);
          }
          break;
      }
    }
  }
  COM_Y_Off();
#if defined(__ARMCC_VERSION)
  printf("e_result = %x , %u\n", e_result, cause);
#else
  printf("e_result = %x , %lu\n", e_result, cause);
#endif /* __ARMCC_VERSION */
  return e_result;
}

/* ========= Private functions for YMODEM send feature ==========*/
/**
  * @brief  Compute CRC16 of a data frame.
  * @param  data pointer to the buffer holding the data frame (128 or 1024)
  * @param  packet_size size of the data buffer
  * @retval COM_StatusTypeDef result of sending via ymodem
  */
static void Calculate_CRC(uint8_t *data, uint32_t packet_size)
{
  uint32_t crc;
  crc = HAL_CRC_Calculate(&CrcHandle, (uint32_t *)data, packet_size);
  data[packet_size] = (crc&0xFF00)>>8;
  data[packet_size+1] = crc&0xFF;
}
/**
  * @brief  Send Ymodem initial frame.
  * @param  fname string of file name to be sent
  * @param  fsize total size of file to be sent
  * @param  buf the buffer to store the packet data for sending
  * @retval COM_StatusTypeDef result of sending via ymodem
  */
static COM_StatusTypeDef Ymodem_SendInitialFrame(const char* fname, const int fsize, uint8_t *buf)
{
  int len = 0;
  uint8_t char2[2] = {0xFF, 0xFF};
  
  /* build header */
  buf[0] = SOH;
  buf[1] = 0x0;
  buf[2] = 0xFF;
  
  /* Fill file name string */
  len=strlen(fname)+1;
  strncpy((char*)&buf[3], fname, len);
  len+=PACKET_HEADER_SIZE;
  
  /* Fill file size string */
  len += Int2DecStr(&buf[len], fsize);
  
  /* Fill in NULL in the remaining bytes */  
  memset(&buf[len], 0x0, PACKET_SIZE + PACKET_HEADER_SIZE - len);
  
  /* Calculate CRC */
  Calculate_CRC(&buf[PACKET_HEADER_SIZE], PACKET_SIZE);
  /* Send header */
  if ( COM_Transmit_Y(buf, PACKET_SIZE + PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE, RX_TIMEOUT) == HAL_OK)
  {    
    COM_Flush();
    if ((COM_Receive_Y(&char2[0], 2U, TX_TIMEOUT) == HAL_OK))
    { 
      if (char2[0] == ACK && char2[1] == CRC16)
        return COM_OK;
    }
    COM_Flush();
    Serial_PutByte(CA);
    Serial_PutByte(CA);
    return COM_ABORT;    
  }
  return COM_ERROR;  
}

/**
  * @brief  Send Ymodem data frame.
  * @param  buf the buffer to store the packet data for sending
  * @param  data pointer to the data to be sent
  * @param  size size of data to be sent (shall be in the range of 1-1024
  * @param  index index of data frame
  * @retval COM_StatusTypeDef result of sending via ymodem
  */
static COM_StatusTypeDef Ymodem_SendDataFrame(uint8_t *buf, 
                                              const uint8_t *data, 
                                              int size, 
                                              const int index)
{
  int len = 0;
  uint8_t char1 = 0xFF;
  
  if ( size == 0 ) return COM_ERROR;
  
  /* build header */
  buf[0] = size < PACKET_SIZE ? SOH : STX;
  buf[1] = index & 0xFF;
  buf[2] = buf[1]^NEGATIVE_BYTE;
  
  /* Fill data */
  memcpy(&buf[PACKET_HEADER_SIZE], data, size);
  len = size < PACKET_SIZE ? PACKET_SIZE - size : PACKET_1K_SIZE - size; 
  
  /* Fill in NULL in the remaining bytes */  
  if (len > 0)
  {
    memset(&buf[PACKET_HEADER_SIZE+size], 0x1A, len);
  }
  
  /* Calculate CRC */
  Calculate_CRC(&buf[PACKET_HEADER_SIZE], size < PACKET_SIZE ? PACKET_SIZE : PACKET_1K_SIZE);
  len = size < PACKET_SIZE ? 
    PACKET_SIZE + PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE :
    PACKET_1K_SIZE + PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE;
  /* Send packet */
  if ( COM_Transmit_Y(buf, len, RX_TIMEOUT) == HAL_OK)
  {
    COM_Flush();
    if ((COM_Receive_Y(&char1, 1U, TX_TIMEOUT) == HAL_OK) )
    {
      if(char1 == ACK)
        return COM_OK;
    
      COM_Flush();
      Serial_PutByte(CA);
      Serial_PutByte(CA);
      return COM_ABORT;
    }
  }
  
  return COM_ERROR;
  
}

/**
  * @brief  Send Ymodem EOT packet and last frame.
  * @param  buf the buffer to store the packet data for sending
  * @retval COM_StatusTypeDef result of sending via ymodem
  */
static COM_StatusTypeDef Ymodem_SendEOTnLastFrame(uint8_t *buf)
{
  uint8_t chars[2] = {0xFF, 0xFF};
  
  COM_Flush();
  /* Send first EOT and wait for NAK */
  Serial_PutByte(EOT);
  if ((COM_Receive_Y(&chars[0], 1U, TX_TIMEOUT) == HAL_OK))
  {
    if (chars[0] == NAK)
    {
      COM_Flush();
      /* Send second EOT and wait for ACK */
      Serial_PutByte(EOT);
      chars[0] = 0xFF;
      if ((COM_Receive_Y(&chars[0], 1U, TX_TIMEOUT) == HAL_OK))
      {
        if ((chars[0] == ACK)) 
        {
          /* Wait for C and send last NULL packet */
          if ((COM_Receive_Y(&chars[0], 1U, TX_TIMEOUT) == HAL_OK))
          {
            if ((chars[0] == CRC16)) 
            {
              buf[0] = SOH;
              buf[1] = 0;
              buf[2] = 0xFF;
              
              /* Fill in 128 NULL bytes */  
              memset(&buf[PACKET_HEADER_SIZE], 0x0, PACKET_SIZE);              
              /* Calculate CRC */
              Calculate_CRC(&buf[PACKET_HEADER_SIZE], PACKET_SIZE);
              /* Send last NULL packet */
              if ( COM_Transmit_Y(buf, PACKET_SIZE + PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE, RX_TIMEOUT) == HAL_OK)
              {    
                COM_Flush();
                chars[0] = 0xFF;
                if ((COM_Receive_Y(&chars[0],1U, TX_TIMEOUT) == HAL_OK))
                { 
                  if (chars[0] == ACK)
                    return COM_OK;
                }
              } /* Transmit the last packet */
            } /* check for C flag */
          } /* wait for C flag*/
        } /* check 2nd EOT ACK */
      } /* wait for 2nd EOT ACK */
    } /* check 1st EOT NAK */
  } /* wait for 1st EOT NAK */
  Serial_PutByte(CA);
  Serial_PutByte(CA);
  return COM_ABORT;
}

/**
  * @brief  Send a file using the ymodem protocol with CRC16.
  * @param  fname string of file name to be sent.
  * @param  fsize size of file binary to be sent.
  * @param  faddr address of the file binary data to be sent. 
  * @retval COM_StatusTypeDef result of sending via ymodem
  */
COM_StatusTypeDef Ymodem_Send(char* fname, int fsize, uint32_t faddr)
{
  uint32_t session_done = 0U, file_done;
  COM_StatusTypeDef e_result = COM_OK;
  int packet_index = 0, remaining_size = fsize;
  uint32_t cause = 0;
  uint8_t char1 = 0;
  
  while ((session_done == 0U) && (e_result == COM_OK))
  {
    /* wait for C to start */
    if ((COM_Receive_Y(&char1, 1U, TX_TIMEOUT) == HAL_OK))
    {
      if(char1== CRC16)
      {
        file_done = 0U;
        
        /* send header and get ACK */
        e_result = Ymodem_SendInitialFrame(fname, fsize, &m_aPacketData[0]);
        cause = 1;
        if ( e_result == COM_OK)
        {
          uint8_t *pdata = (uint8_t*)faddr;
          while ((file_done == 0U) && (e_result == COM_OK))
          {
            packet_index++;
            if ( remaining_size > PACKET_1K_SIZE)
            {
              /* send 1K packet and get ACK */
              e_result = Ymodem_SendDataFrame(&m_aPacketData[0], pdata, PACKET_1K_SIZE, packet_index);
              cause = 2;
              if ( e_result == COM_OK)
              {
                remaining_size-=PACKET_1K_SIZE;
                pdata+=PACKET_1K_SIZE;
              }
              else
              {
                break;
              }          
            }
            else
            {
              /* send last packet and get ACK*/
              e_result = Ymodem_SendDataFrame(&m_aPacketData[0], pdata, remaining_size, packet_index);
              cause = 3;
              if ( e_result == COM_OK)
              {
                /* send EOT */
                e_result = Ymodem_SendEOTnLastFrame(&m_aPacketData[0]);
                cause = 4;
                if ( e_result == COM_OK )
                {
                  file_done = 1;
                  session_done = 1;
                }
              }
              else
              {
                break;
              }     
            }          
          }
        } /* If header send OK*/
        
        if(e_result != COM_OK)
        {
          session_done = 1;
        }
      } /*if(chars[0] == CRC16)*/
    } /*if ((COM_Receive_Y(&chars[0], 1U, TX_TIMEOUT) == HAL_OK))*/
  } /* while ((session_done == 0U) && (e_result == COM_OK))*/
  
  COM_Y_Off();
  if(e_result == COM_OK)
  {
    printf("Image sent via Ymodem successfully!\r\n");
  }
  else
  {
    printf("Image sent via Ymodem failed!\r\n");
  }
#if defined(__ARMCC_VERSION)
  printf("e_result = %x , %u\r\n", e_result, cause);
#else
  printf("e_result = %x , %lu\r\n", e_result, cause);
#endif /* __ARMCC_VERSION */
  return e_result;
}

COM_StatusTypeDef Ymodem_SendWithCB(char* fname, int fsize, int(*cb)(uint8_t *pbuf, int size, int offset))
{
  uint32_t session_done = 0U, file_done;
  COM_StatusTypeDef e_result = COM_OK;
  int packet_index = 0, offset = 0, read_size, return_size;
  uint32_t cause = 0;
  uint8_t char1 = 0;
  if( fname == NULL || fsize<=0 || cb == NULL)
  {
    return COM_ERROR;
  }
  
  while ((session_done == 0U) && (e_result == COM_OK))
  {
    /* wait for C to start */
    if ((COM_Receive_Y(&char1, 1U, TX_TIMEOUT) == HAL_OK))
    {
      if(char1== CRC16)
      {
        file_done = 0U;
        
        /* send header and get ACK */
        e_result = Ymodem_SendInitialFrame(fname, fsize, &m_aPacketData[0]);
        cause = 1;
        if ( e_result == COM_OK)
        {
          while ((file_done == 0U) && (e_result == COM_OK))
          {
            packet_index++;
            read_size = fsize - offset;
            read_size = read_size >= PACKET_1K_SIZE ? PACKET_1K_SIZE : read_size;
            if (read_size >0)
            {
              return_size = cb(&m_aReadData[0], read_size, offset);
              if (return_size != read_size)
              {
                cause = 0xff;
                session_done = 1;
                break;
              }
            }
          
            if ( read_size == PACKET_1K_SIZE)
            {
              /* send 1K packet and get ACK */
              e_result = Ymodem_SendDataFrame(&m_aPacketData[0], &m_aReadData[0], PACKET_1K_SIZE, packet_index);
              cause = 2;
              if ( e_result == COM_OK)
              {
                offset+=read_size;
              }
              else
              {
                break;
              }          
            }
            else
            {
              if (read_size >0)
              {
                /* send last packet and get ACK*/
                e_result = Ymodem_SendDataFrame(&m_aPacketData[0], &m_aReadData[0], read_size, packet_index);
                cause = 3;
              }
              
              if ( e_result == COM_OK)
              {
                /* send EOT */
                e_result = Ymodem_SendEOTnLastFrame(&m_aPacketData[0]);
                cause = 4;
                if ( e_result == COM_OK )
                {
                  file_done = 1;
                  session_done = 1;
                }
              }
              else
              {
                break;
              }     
            }          
          }
        } /* If header send OK*/
        
        if(e_result != COM_OK)
        {
          session_done = 1;
        }
      } /*if(chars[0] == CRC16)*/
    } /*if ((COM_Receive_Y(&chars[0], 1U, TX_TIMEOUT) == HAL_OK))*/
  } /* while ((session_done == 0U) && (e_result == COM_OK))*/
  
  COM_Y_Off();
  if(e_result == COM_OK)
  {
    printf("Image sent via Ymodem successfully!\r\n");
  }
  else
  {
    printf("Image sent via Ymodem failed!\r\n");
  }
#if defined(__ARMCC_VERSION)
  printf("e_result = %x , %u\r\n", e_result, cause);
#else
  printf("e_result = %x , %lu\r\n", e_result, cause);
#endif /* __ARMCC_VERSION */
  return e_result;
}
/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
