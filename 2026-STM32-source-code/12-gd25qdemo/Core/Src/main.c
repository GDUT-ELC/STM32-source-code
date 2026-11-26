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
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_flash.h"   /* 应用层封装：init / save / load / erase / self_test */
#include "OLED.h"        
/* 需要更底层接口时用 gd25q80.h（在 Bsp 目录） */
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

/* USER CODE END PV *//* USER CODE END PV */

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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
	
	
  /* ================== 在这里写你自己的功能 ==================
   * 外部 Flash 的功能已经封装在 app_flash.c/h 里，直接调用：
   *
   *   app_flash_init();                     初始化，上电先调一次
   *   app_flash_save(地址, 数据, 长度);      保存数据，自动擦除
   *   app_flash_load(地址, 缓冲区, 长度);    读取数据
   *   app_flash_erase(地址);                擦除 4KB 扇区
   *   app_flash_read_id(&id);               读芯片 ID，正常 0xC84014
   *   app_flash_self_test();                自检：写一段再读回比对
   *
   * 最小示例（初始化并自检，返回 0 表示成功）：
   *   GD25Q_Status ret = app_flash_init();
   *   if (ret == GD25Q_OK) ret = app_flash_self_test();
   *
   * 调试小技巧：Keil 进入调试后，在 Watch 窗口添加变量 g_flash_info，
   *   不用加打印就能看到芯片 ID、最近一次的结果、地址和长度；
   *   g_flash_info.step 的数字含义见 app_flash.h 里的 app_flash_step_t。
   *
   * 想把返回值显示出来，用 GD25Q_StatusString(返回值)，
   *   它会给出 OK、RANGE 这类英文短词；每个值的中文含义见 app_flash.h。
   * 完整接口见 app_flash.h，底层接口见 gd25q80.h
   * ========================================================= */


  /* ===== 临时测试代码开始：用完可删掉本块 ===== */
  {
      uint8_t t_w[4] = {0x11, 0x22, 0x33, 0x44};
      uint8_t t_r[4] = {0x00, 0x00, 0x00, 0x00};
      uint8_t t_same = 0;
      GD25Q_Status t_ret;

      oled_init();
      oled_clear();

      t_ret = app_flash_init();
      if (t_ret == GD25Q_OK)
      {
          t_ret = app_flash_save(0x001000, t_w, 4);
      }
      if (t_ret == GD25Q_OK)
      {
          t_ret = app_flash_load(0x001000, t_r, 4);
      }

      t_same = (t_r[0] == t_w[0]) && (t_r[1] == t_w[1]) &&
               (t_r[2] == t_w[2]) && (t_r[3] == t_w[3]);

      oled_showstring(1, 1, "API TEST");
      oled_showstring(2, 1, (char *)GD25Q_StatusString(t_ret));
      oled_showstring(3, 1, t_same ? "DATA PASS" : "DATA FAIL");
  }
  /* ===== 临时测试代码结束 ===== */
	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 在这里添加循环执行的功能。保存数据用 GD25Q_Write()，读取用 GD25Q_ReadData()。
     * 不要在循环里反复擦写，Flash 擦写寿命有限，仅在数据变化时写入 */
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
