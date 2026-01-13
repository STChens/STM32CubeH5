/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define RESET_n_INIT_COUNTER()  \
  {     \
      DWT->CYCCNT = 0;  \
      DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;\
  }
#define STOP_n_GET_COUNTER(val)  \
  {     \
    DWT->CTRL &= (~DWT_CTRL_CYCCNTENA_Msk); \
    (val) = DWT->CYCCNT; \
  }
#define ENABLE_DWT_COUNTER()    \
  {\
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;\
    DWT->CYCCNT = 0;\
  }

#define Input_64K 0x08100000
#define Size_64K     (0x10000)
#define Size_63K     (0x10000 - 0x400)
#define Size_65K     (0x10000 + 0x400)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

static HASH_HandleTypeDef hhash;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* USER CODE BEGIN PV */
static __ALIGN_BEGIN const __IO uint8_t aInput[] __ALIGN_END = "The hash processor is a fully compliant implementation of the secure hash algorithm (SHA-1, SHA-2 family) and the HMAC (keyed-hash message authentication code) algorithm.";
__ALIGN_BEGIN static uint8_t aSHA224Digest[28] __ALIGN_END;
__ALIGN_BEGIN static uint8_t aExpectSHA224Digest[28] __ALIGN_END = { 0x61, 0x30, 0x9d, 0x5f, 0xa1, 0xe7, 0x82, 0x88,
                                                                     0x98, 0x20, 0xfc, 0xff, 0xc4, 0x62, 0x46, 0x72,
                                                                     0x63, 0xd4, 0xe1, 0x9c, 0xa4, 0x6d, 0xac, 0x17,
                                                                     0x7b, 0x8f, 0x05, 0x0a
                                                                    };
__ALIGN_BEGIN static uint8_t aSHA256Digest[32] __ALIGN_END;
__ALIGN_BEGIN static uint8_t aExpectSHA256Digest[32] __ALIGN_END = { 0x6d, 0x57, 0xa0, 0x56, 0xac, 0xa1, 0x3b, 0xcc,
                                                                     0x9f, 0x6a, 0x9a, 0xb4, 0xce, 0x90, 0x0a, 0x28,
                                                                     0xbe, 0xb5, 0xd8, 0x60, 0x70, 0x14, 0xb3, 0xe3,
                                                                     0xc0, 0xb3, 0xb3, 0xd7, 0xbd, 0x2e, 0x62, 0xf4
							            };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void GPDMA1_Init(void);
static void GPDMA1_Init_LinkList(uint32_t start_addr, int size);
static void HASH_Init(int is_224);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void DemoPrintCounter(uint32_t cnt, char *str, int mode, int buf_size);
void print_buf(const char * str, const uint8_t *buf, const size_t size, int showstring);

/**
  * @brief  Print clock counter number and convert to ms/us
  * @param  cnt clock counter
  * @param  str string to be printed at the beginning
  * @param  keylen length of cipher key
  * @retval None.
  */
static void DemoPrintCounter(uint32_t cnt, char *str, int mode, int buf_size)
{
  printf("\r\n------------------------------------------------------\r\n");
  if((str)!=NULL)
    printf("\t %s:\r\n", str);
  printf("Mode: %d\r\n", mode);
  printf("Buffer size: %d\r\n", buf_size);
  printf("Clock cycle: %d\r\n", cnt);
  cnt=cnt>>4;
  cnt=cnt/10;
  printf("Time: %d(ms) %d(us)\r\n", cnt/1000, cnt);
  printf("------------------------------------------------------\r\n");
}

/**
  * @brief  Print data buffer content
  * @param  str string to be printed at the beginning
  * @param  buf pointer to the buffer holding the data to print
  * @param  size size of the data in buffer in bytes
  * @retval None.
  */
void print_buf(const char * str, const uint8_t *buf, const size_t size, int showstring)
{
  int i;
  if(str!=NULL)
  {
    printf("%s", str);
    printf("\r\n------------------------------------\r\n");
  }
  for (i = 0; i< size; i++)
  {
    if(i%16 == 0)
      printf("%08x:  ", i);

    printf("%02x ",buf[i]);
    if((i+1)%8 == 0) printf(" ");
    if((i+1)%16 == 0) {
      if ( showstring != 0 )
      {
        int j;
        printf("| ");
        for ( j = i-16; j<i; j++)
        {
          printf("%c", ((buf[j]>=0x20) && (buf[j]<127))?buf[j]:' ');
        }
      }
      printf("\r\n");
    }
  }
  if(str!=NULL)
    printf("\r\n------------------------------------\r\n");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
static void hash_test_dma_224_256(int is_224, uint8_t * buf, int buf_size, uint8_t * ref, int ref_size, uint8_t *outbuf)
{
  uint32_t cnt = 0;

  memset(outbuf, 0, ref_size);

	printf("\r\n ===> Test HASH DMA mode %s\r\n", is_224 == 1?"SHA224":"SHA256");
	/* Initialize all configured peripherals */
        if ( buf_size < Size_64K )
        {
          GPDMA1_Init();
        }
        else
        {
          printf("Size must be < 64KB when using DMA mode\r\n");
          return ;
          // GPDMA1_Init_LinkList((uint32_t)buf, buf_size);	  
        }
        HASH_Init(is_224);

	  printf("%s:%d\r\n", __FUNCTION__, __LINE__);
	  RESET_n_INIT_COUNTER();

	  /* Start HASH computation using DMA transfer */
	  if (HAL_HASH_Start_DMA(&hhash, (uint8_t*)buf, buf_size, outbuf) != HAL_OK)
	  {
		  printf("Call HAL_HASH_Start_DMA failed!\r\n");

	    Error_Handler();
	  }
	  /* Wait for DMA transfer to complete */
	  while (HAL_HASH_GetState(&hhash) == HAL_HASH_STATE_BUSY);
	  STOP_n_GET_COUNTER(cnt);
	  DemoPrintCounter(cnt, "HASH computation", is_224 == 1 ? 224 : 256, buf_size);

	  printf("%s:%d HASH computation with DMA finished\r\n", __FUNCTION__, __LINE__);
	  print_buf("Computed hash: ", outbuf, ref_size, 0);
	  if ( ref != NULL )
	  {
		  print_buf("Expected hash: ", ref, ref_size, 0);

		 /* Compare computed digest with expected one */
		  if( memcmp(outbuf, ref, ref_size) != 0)
		  {
			  printf("HASH computation result not as expected!\r\n");

			BSP_LED_On(LED2);
		  }
		  else
		  {
			  printf("HASH computation result is OK!\r\n");
			BSP_LED_On(LED1);
		  }
	  }
	  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

	  if(HAL_HASH_DeInit(&hhash) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  HAL_DMA_DeInit(hhash.hdmain);
	  printf("<=== Deinit and return\r\n\r\n");
}

void hash_test(void)
{
#if 0
  /****************************************************************************/
  /****************************** SHA224 **************************************/
  /****************************************************************************/
	hash_test_dma_224_256(1,
			(uint8_t*)aInput, strlen((char const*)aInput),
			aExpectSHA224Digest, sizeof(aExpectSHA224Digest)/sizeof(aExpectSHA224Digest[0]),
			aSHA224Digest);

  /****************************************************************************/
  /***************************** SHA256 ***************************************/
  /****************************************************************************/

	hash_test_dma_224_256(0,
			(uint8_t*)aInput, strlen((char const*)aInput),
			aExpectSHA256Digest, sizeof(aExpectSHA256Digest)/sizeof(aExpectSHA256Digest[0]),
			aSHA256Digest);
#endif
  /****************************************************************************/
  /****************************** SHA224 **************************************/
  /****************************************************************************/
	hash_test_dma_224_256(1,
			(uint8_t*)Input_64K, Size_64K,
			NULL, sizeof(aExpectSHA224Digest)/sizeof(aExpectSHA224Digest[0]),
			aSHA224Digest);
	hash_test_dma_224_256(1,
			(uint8_t*)Input_64K, Size_64K,
			NULL, sizeof(aExpectSHA224Digest)/sizeof(aExpectSHA224Digest[0]),
			aSHA224Digest);
	hash_test_dma_224_256(1,
			(uint8_t*)Input_64K, Size_65K,
			NULL, sizeof(aExpectSHA224Digest)/sizeof(aExpectSHA224Digest[0]),
			aSHA224Digest);
  /****************************************************************************/
  /***************************** SHA256 ***************************************/
  /****************************************************************************/
	hash_test_dma_224_256(0,
			(uint8_t*)Input_64K, Size_63K,
			NULL, sizeof(aExpectSHA256Digest)/sizeof(aExpectSHA256Digest[0]),
			aSHA256Digest);
	hash_test_dma_224_256(0,
			(uint8_t*)Input_64K, Size_64K,
			NULL, sizeof(aExpectSHA256Digest)/sizeof(aExpectSHA256Digest[0]),
			aSHA256Digest);
	hash_test_dma_224_256(0,
			(uint8_t*)Input_64K, Size_65K,
			NULL, sizeof(aExpectSHA256Digest)/sizeof(aExpectSHA256Digest[0]),
			aSHA256Digest);
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */
	/* HASH DMA Init */
	/* GPDMA1_REQUEST_HASH_IN Init */
	handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
	handle_GPDMA1_Channel0.Init.Request = GPDMA1_REQUEST_HASH_IN;
	handle_GPDMA1_Channel0.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	handle_GPDMA1_Channel0.Init.Direction = DMA_MEMORY_TO_PERIPH;
	handle_GPDMA1_Channel0.Init.SrcInc = DMA_SINC_INCREMENTED;
	handle_GPDMA1_Channel0.Init.DestInc = DMA_DINC_FIXED;
	handle_GPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
	handle_GPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
	handle_GPDMA1_Channel0.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
	handle_GPDMA1_Channel0.Init.SrcBurstLength = 4;
	handle_GPDMA1_Channel0.Init.DestBurstLength = 4;
	handle_GPDMA1_Channel0.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT1;
	handle_GPDMA1_Channel0.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
	handle_GPDMA1_Channel0.Init.Mode = DMA_NORMAL;
	if (HAL_DMA_Init(&handle_GPDMA1_Channel0) != HAL_OK)
	{
	  Error_Handler();
	}

	__HAL_LINKDMA(&hhash, hdmain, handle_GPDMA1_Channel0);
  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

#define DMA_BLOCK_SIZE 0x8000 // max size of block is 32KB
#define DMA_MAX_NODES 32
DMA_QListTypeDef Queue = {0};
DMA_NodeTypeDef Node[DMA_MAX_NODES];    
    
/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void GPDMA1_Init_LinkList(uint32_t start_addr, int size)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */
    /* HASH DMA Init */
    /* GPDMA1_REQUEST_HASH_IN Init */
    handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
    handle_GPDMA1_Channel0.InitLinkedList.Priority = DMA_HIGH_PRIORITY;
    handle_GPDMA1_Channel0.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    handle_GPDMA1_Channel0.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    handle_GPDMA1_Channel0.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
    handle_GPDMA1_Channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;
    if (HAL_DMAEx_List_Init(&handle_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }
    /* Build link list nodes */
    int off = 0, i = 0;
    HAL_StatusTypeDef ret = HAL_OK;
    DMA_NodeConfTypeDef pNodeConfig;

    /* Set node configuration ################################################*/
    pNodeConfig.NodeType = DMA_CHANNEL_TYPE_GPDMA;
    pNodeConfig.Init.Request = GPDMA1_REQUEST_HASH_IN;
    pNodeConfig.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    pNodeConfig.Init.Direction = DMA_MEMORY_TO_PERIPH;
    pNodeConfig.Init.SrcInc = DMA_SINC_INCREMENTED;
    pNodeConfig.Init.DestInc = DMA_DINC_FIXED;
    pNodeConfig.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    pNodeConfig.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    pNodeConfig.Init.SrcBurstLength = 4;
    pNodeConfig.Init.DestBurstLength = 4;
    pNodeConfig.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT1;
    pNodeConfig.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    pNodeConfig.Init.Mode = DMA_NORMAL;
    pNodeConfig.RepeatBlockConfig.RepeatCount = 1;
    pNodeConfig.RepeatBlockConfig.SrcAddrOffset = 0;
    pNodeConfig.RepeatBlockConfig.DestAddrOffset = 0;
    pNodeConfig.RepeatBlockConfig.BlkSrcAddrOffset = 0;
    pNodeConfig.RepeatBlockConfig.BlkDestAddrOffset = 0;
    pNodeConfig.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    pNodeConfig.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    pNodeConfig.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    
    for ( off = 0; off < size; off += DMA_BLOCK_SIZE )
    {
        pNodeConfig.SrcAddress = (uint32_t) start_addr + off;
        pNodeConfig.DstAddress = (uint32_t) HASH->DIN;
        pNodeConfig.DataSize = size - off < DMA_BLOCK_SIZE ? size - off : DMA_BLOCK_SIZE;

        /* Build Node1 Node */
        ret |= HAL_DMAEx_List_BuildNode(&pNodeConfig, &Node[i]);

        /* Insert Node1 to Queue */
        ret |= HAL_DMAEx_List_InsertNode_Tail(&Queue, &Node[i]); 
        i++;
    }
    if (HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel0, &Queue) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&hhash, hdmain, handle_GPDMA1_Channel0);
  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief HASH Initialization Function
  * @param None
  * @retval None
  */
static void HASH_Init(int is_224)
{

  /* USER CODE BEGIN HASH_Init 0 */

  /* USER CODE END HASH_Init 0 */

  /* USER CODE BEGIN HASH_Init 1 */

  /* USER CODE END HASH_Init 1 */
  hhash.Instance = HASH;
  hhash.Init.DataType = HASH_BYTE_SWAP;
  if ( is_224 == 1 )
	  hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA224;
  else
	  hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA256;
  if (HAL_HASH_Init(&hhash) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN HASH_Init 2 */

  /* USER CODE END HASH_Init 2 */

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

