/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    GPIO/GPIO_IOToggle/Src/main.c
  * @author  MCD Application Team
  * @brief   This example describes how to configure and use GPIOs through
  *          the STM32H5xx HAL API.
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "low_level_obkeys.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "com.h"
#include "common.h"
#include "ymodem.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static GPIO_InitTypeDef  GPIO_InitStruct;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ICACHE_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void open_debug(void);
static void read_SBS_reg(void);
static void SetProductState(uint32_t prodState);
static void GetProductState(void);

static void read_SBS_reg(void)
{
  __HAL_RCC_SBS_CLK_ENABLE();
  printf("READ SBS HDPL and DBG registers:\r\n");
  printf("SBS DBGCR [0x%08x] \r\n", SBS->DBGCR);
  printf("SBS DBGLOCKR [0x%08x] \r\n", SBS->DBGLOCKR);
  printf("SBS HDPLCR [0x%08x] \r\n", SBS->HDPLCR);
  printf("SBS HDPLSR [0x%08x] \r\n", SBS->HDPLSR);
  printf("SBS NEXTHDPLCR [0x%08x] \r\n", SBS->NEXTHDPLCR);
  printf("DBGMCU CR [0x%08x] \r\n", DBGMCU->CR);
  printf("DBGMCU SR [0x%08x] \r\n", DBGMCU->SR);
}

static void open_debug(void)
{
  __HAL_RCC_SBS_CLK_ENABLE();

  printf("Test SBS DBG \r\n");
  printf("==> Read SBS DBG CR register [%08x]\r\n", SBS->DBGCR);
  printf("==> Enable AP and DBG from SBS\r\n");
  SBS->DBGCR = 0xb451b4b4;
  SBS->DBGLOCKR = 0x0000006a;
  DBGMCU->CR |= 0x00010000; /* set bit16 as 1 to reset SBS under power reset instead of system reset*/
  read_SBS_reg();
  while(1){}
}

static void GetProductState(void)
{
  FLASH_OBProgramInitTypeDef flash_option_bytes_bank1 = {0};
  uint8_t ps = 0;
  
  flash_option_bytes_bank1.Banks = FLASH_BANK_1;
  HAL_FLASHEx_OBGetConfig(&flash_option_bytes_bank1);
  
  ps = (uint8_t)((flash_option_bytes_bank1.ProductState & 0xFF00)>>8);
  printf("Current Product state is %02x  ",ps);
  
  switch (ps)
  {
    case 0xED: printf("\t : << OPEN >> \r\n"); break;
    case 0x17: printf("\t : << PROVISIONING >> \r\n"); break;
    case 0x2E: printf("\t : << PROVISIONED >> \r\n"); break;
    case 0xC6: printf("\t : << TZ-CLOSED >> \r\n"); break;
    case 0x72: printf("\t : << CLOSED >> \r\n"); break;
    case 0x5C: printf("\t : << LOCKED >> \r\n"); break;
    default: printf("\t UNKNOWN!!\r\n"); break;    
  }
}


static void SetProductState(uint32_t prodState)
{
  FLASH_OBProgramInitTypeDef flash_option_bytes_bank1 = {0};
  HAL_StatusTypeDef ret = HAL_ERROR;
  //flash_option_bytes_bank1.Banks = FLASH_BANK_1;
  //flash_option_bytes_bank1.BootConfig = OB_BOOT_SEC;
  //HAL_FLASHEx_OBGetConfig(&flash_option_bytes_bank1);

  printf("Setting product state to 0x%x ...\r\n", prodState);

   /* Unlock the Flash to enable the flash control register access */
  HAL_FLASH_Unlock();

  /* Unlock the Options Bytes */
  HAL_FLASH_OB_Unlock();

  flash_option_bytes_bank1.OptionType = OPTIONBYTE_PROD_STATE;
  flash_option_bytes_bank1.ProductState = prodState;

  printf("Program state ...\r\n");

  ret = HAL_FLASHEx_OBProgram(&flash_option_bytes_bank1);
  if (ret != HAL_OK)
  {
    printf("Error while setting OB Bank1 config state!\r\n");
    Error_Handler();
  }

  printf("OB Launch ...");

  /* Launch the Options Bytes (reset the board, should not return) */
  ret = HAL_FLASH_OB_Launch();
  if (ret != HAL_OK)
  {
    printf("Error while execution OB_Launch\r\n");
    Error_Handler();
  }

  printf("After OB_Launch: system reset !\r\n");  
  HAL_Delay(500);
  NVIC_SystemReset();
}

static void uart_putc(unsigned char c)
{
  COM_Transmit(&c, 1, 1000U);
}

/* Redirects printf to TFM_DRIVER_STDIO in case of ARMCLANG*/
#if defined(__ARMCC_VERSION)
FILE __stdout;

/* __ARMCC_VERSION is only defined starting from Arm compiler version 6 */
int fputc(int ch, FILE *f)
{
  /* Send byte to USART */
  uart_putc(ch);

  /* Return character written */
  return ch;
}
#elif defined(__GNUC__)
/* Redirects printf to TFM_DRIVER_STDIO in case of GNUARM */
int _write(int fd, char *str, int len)
{
  int i;

  for (i = 0; i < len; i++)
  {
    /* Send byte to USART */
    uart_putc(str[i]);
  }

  /* Return the number of characters written */
  return len;
}
#elif defined(__ICCARM__)
int putchar(int ch)
{
  /* Send byte to USART */
  uart_putc(ch);

  /* Return character written */
  return ch;
}
#endif /*  __GNUC__ */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* STM32H5xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
             can eventually implement his proper time base source (a general purpose
             timer for example or other time source), keeping in mind that Time base
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
             handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  //SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ICACHE_Init();
  /* USER CODE BEGIN 2 */

   /* -1- Enable GPIO Clock (to be able to program the configuration registers) */
  LED1_GPIO_CLK_ENABLE();
  LED2_GPIO_CLK_ENABLE();

  /* -2- Configure IO in output push-pull mode to drive external LEDs */
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  GPIO_InitStruct.Pin = LED1_PIN;
  HAL_GPIO_Init(LED1_GPIO_PORT, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = LED2_PIN;
  HAL_GPIO_Init(LED2_GPIO_PORT, &GPIO_InitStruct);
  
  /*
  COM_InitTypeDef COM_Init;
  COM_Init.BaudRate = 115200;
  COM_Init.HwFlowCtl = COM_HWCONTROL_NONE;
  COM_Init.Parity = COM_PARITY_NONE;
  COM_Init.StopBits = COM_STOPBITS_1;
  COM_Init.WordLength = COM_WORDLENGTH_8B;  
  BSP_COM_Init(COM1, &COM_Init);
  */
  COM_Init();
  
  
  {
    //extern UART_HandleTypeDef hcom_uart[COM_NBR];
    int loop=1;

      // Check UART
    
    while(loop == 1)
    {
      uint8_t response;
      printf("\r\n ========== Test menu ========== \r\n");
      printf("Open debug                 ---------- d\n");
      printf("Provision default DA obk   ---------- p\n");
      printf("Provision obk from ymodem  ---------- q\n");
      printf("Regression to open         ---------- r\n");
      printf("Show obk (HDPL1/2/3)       ---------- s\n");
      printf("Increment HDPL             ---------- h\n");
      printf("Increment OBKHDPL          ---------- o\n");
      printf("Set HDPL to Level3         ---------- i\n");
      printf("Get current product state  ---------- 0\n");
      printf("Set state to provisioning  ---------- 1\n");
      printf("Set state to provisioned   ---------- 2\n");
      printf("Set state to closed        ---------- 3\n");
      printf("Exit                       ---------- x\n");
      //while (HAL_UART_Receive(&hcom_uart[COM1], &response, 1, 1000) == HAL_TIMEOUT);
      COM_Flush();
      while (COM_Receive(&response, 1, 1000) == HAL_TIMEOUT);
      printf("Your input is : %c\r\n", response);
      switch (response)
      {
      case 'i':
        __HAL_RCC_SBS_CLK_ENABLE();
        printf("Current HDPL level is %02X\r\n", SBS->HDPLSR);
        if(SBS->HDPLSR == 0x51)
        {
          printf("===> Set HDPL from level 1 to level 3!\r\n");
          SBS->HDPLCR = 0x6A;
          SBS->HDPLCR = 0x6A;
        }
        else if(SBS->HDPLSR == 0x8A)
        {
          printf("===> Set HDPL from level 2 to level 3!\r\n");
          SBS->HDPLCR = 0x6A;
        }
        else if(SBS->HDPLSR == 0x6F)
        {
          printf("HDPL is already in level 3, nothing to do\r\n");
        }
        printf("HDPL level now is %02X\r\n", SBS->HDPLSR);
        break;
      case 'd':
        printf("===> Recovery => open debug!\r\n");
        open_debug();
      case 'p':
        printf("===> Force Provision of DA obk\r\n");
        OBK_ProvisionObk(0x1);
        break;
      case 'q':
        printf("===> Provision obk from ymodem\r\n");
        OBK_ProvisionObk(0x3);
        break;
      case 'r':
        printf("===> Launch regression\r\n");
        SetProductState(OB_PROD_STATE_REGRESSION);
        break;
      case 's':
        printf("===> Read DA obk area\r\n");
        OBK_ReadObk(0x100, 128);
        OBK_ReadObk(0x900, 128);
        OBK_ReadObk(0xC00, 128);
        break;
      case 'h':
        printf("===> Increment HDPL\r\n");
        __HAL_RCC_SBS_CLK_ENABLE();
        HAL_SBS_IncrementHDPLValue();
        break;
      case 'o':
        printf("===> Increment OBKHDPL\r\n");
        __HAL_RCC_SBS_CLK_ENABLE();
        HAL_SBS_SetOBKHDPL(HAL_SBS_GetOBKHDPL()+1);
        break;
      case '0':
        printf("===> Get current product state \r\n");
        GetProductState();
        break;
      case '1':
        printf("===> set product state to provisioning \r\n");
        SetProductState(OB_PROD_STATE_PROVISIONING);
        break;
      case '2':
        printf("===> set product state to provisioned \r\n");
        SetProductState(OB_PROD_STATE_IROT_PROVISIONED);
        break;
      case '3':
        printf("===> set product state to closed \r\n");
        SetProductState(OB_PROD_STATE_CLOSED);
        break;
      case 'x':
        loop = 0;
        break;
      default :
        printf("===> Bad input!\r\n");
        break;
      }
    }
  }
  printf(" Exit from menu, start GPIO toggle \r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
    /* Insert delay 100 ms */
    HAL_Delay(100);
    HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
    /* Insert delay 100 ms */
    HAL_Delay(100);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS_DIGITAL;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 250;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache (default 2-ways set associative cache)
  */
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
