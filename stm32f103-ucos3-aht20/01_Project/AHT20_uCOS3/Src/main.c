/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "gpio.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "board_config.h"
#include "bsp_i2c.h"
#include "app_tasks.h"
#include "os.h"
#include "diag.h"
#include "bsp_uart.h"
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  OS_ERR os_err;

  /* ================= 上电自检（排错第一现场） =================
   * 闪 3 下 = CPU 在跑、时钟配置成功、PC13 能点（灯亮不亮都要闪）
   * 然后打一行 [DIAG]，这一行出现在串口助手里 = PA9 + USB-TTL + 波特率都对
   * 如果连这 3 下都没有：问题不在内核，去看 Boot0 跳线 / 下载 / 复位。
   * ============================================================ */
#if (APP_DIAG_BOOT != 0)
  DIAG_Blink(3U, 120U, 120U);
  BSP_UART_Printf("\r\n[DIAG] CPU alive, HCLK=%lu Hz, USART1 ready\r\n",
                  (unsigned long)HAL_RCC_GetHCLKFreq());
#endif

#if (APP_DIAG_BARE != 0)
  DIAG_BareLoop();          /* 裸机自检：永不返回 */
#endif

  /* I2C 传输层：硬件 I2C1 或 GPIO 模拟，由 board_config.h 的 APP_I2C_USE_SOFT 选择 */
  if (BSP_I2C_Init() == 0U)
  {
    Error_Handler();
  }

  /* ---------------------------------------------------------------
   * uC/OS-III 初始化三步曲
   *   1) OSInit()      : 初始化内核（就绪表、空闲任务、统计任务、定时器任务）
   *   2) 创建用户任务   : 本项目在 App_TasksInit() 里用 OSTaskCreate 建 3 个任务
   *   3) OSStart()     : 启动调度器，从此永不返回
   * 注意：OSStart() 之前必须先给 SysTick 装好节拍（OS_CPU_SysTickInit），
   *       同时 SysTick_Handler/PendSV_Handler 已在 stm32f1xx_it.c 里交给内核。
   * --------------------------------------------------------------- */
#if (APP_DIAG_BOOT != 0)
  BSP_UART_Printf("[CHK] before OSInit\r\n");
#endif
  OSInit(&os_err);
  if (os_err != OS_ERR_NONE)
  {
    Error_Handler();
  }
#if (APP_DIAG_BOOT != 0)
  BSP_UART_Printf("[CHK] OSInit ok, err=%d\r\n", (int)os_err);
#endif

  /* 创建 3 个任务并打印开机信息 */
  App_TasksInit();
#if (APP_DIAG_BOOT != 0)
  BSP_UART_Printf("[CHK] App_TasksInit returned\r\n");
#endif

  /* 节拍 = HCLK / OS_CFG_TICK_RATE_HZ = 8MHz / 1000 = 8000 个计数。
     这里不用 SystemCoreClock 变量，而用寄存器实时算出来的 HAL_RCC_GetHCLKFreq()，
     免得时钟切换后变量没更新，节拍悄悄变成 9 倍慢（表现为"灯很久才动一下"）。 */
  OS_CPU_SysTickInit((CPU_INT32U)(HAL_RCC_GetHCLKFreq() / OS_CFG_TICK_RATE_HZ));
  BSP_UART_Printf("[%8lu ms] OS     systick reload = %lu, OSStart() ...\r\n",
                  (unsigned long)HAL_GetTick(),
                  (unsigned long)(HAL_RCC_GetHCLKFreq() / OS_CFG_TICK_RATE_HZ));

  /* 启动调度器：正常情况下永远不返回 */
  OSStart(&os_err);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 走到这里说明调度器意外退出了，正常不会发生 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  /* 停机前先让 PC13 快速闪 1 下/停 0.7s：一眼看出"程序跑到了 Error_Handler" */
  DIAG_Panic(1U);
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
