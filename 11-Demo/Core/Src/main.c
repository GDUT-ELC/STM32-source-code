/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "buzzer.h"
#include "led.h"
#include "key.h"
#include "adc.h"
#include "hcsr04.h"
#include "MPU6050.h"
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
float distance;                         // 距离变量(cm)
uint32_t ldr_value;                     // LDR原始值
float voltage;                          // LDR电压值(V)
uint8_t display_mode;                   // 显示模式(0:距离 1:LDR)
int16_t AX, AY, AZ, GX, GY, GZ;			//定义用于存放各个数据的变量
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
  display_mode = 0;
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
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  oled_init();
  MPU6050_Init();
  led_blue_off();
  
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
  
  hcsr04_start();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	uint8_t key = key_getnum();
    if (key == 1)
    {
		display_mode++;      // 切换显示模式
		if (display_mode > 2)
		{
			display_mode = 0;
		}
		
		buzzer_on();
		HAL_Delay(100);
		buzzer_off();
		oled_clear();
    }
	
	// 读取LDR数据
	HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    ldr_value = HAL_ADC_GetValue(&hadc1);
    voltage = (ldr_value / 4095.0f) * 3.3f;
	
	// 超声波测距
    distance = hcsr04_getdistance(&HCSR04);
	hcsr04_start();
	
	// 读取陀螺仪数据
	MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);
	
	if (distance < 20.0f && distance > 0.0f)
    {
		led_red_on();
		led_green_off();
		if (distance < 10.0f)
		{
			buzzer_on();        // 近距离蜂鸣报警
		}
		else
		{
			buzzer_off();
		}
    }
    else
    {
		led_red_off();
		led_green_on();
		buzzer_off();
    }
	
	// OLED显示
    oled_showstring(1, 1, "Mode:");
    if (display_mode == 0)
    {
		oled_showstring(1, 6, "Distance");
		oled_showstring(2, 1, "Dist:");
		oled_shownum(2, 6, (uint32_t)distance, 3);
		oled_showchar(2, 9, 'c');
		oled_showchar(2, 10, 'm');
    }
    else if (display_mode == 1)
    {
		oled_showstring(1, 6, "LDR Sensor");
		oled_showstring(2, 1, "ADC:");
		oled_shownum(2, 5, ldr_value, 4);
		oled_showstring(3, 1, "Volt:");
		oled_shownum(3, 6, (uint32_t)(voltage * 1000), 3);
		oled_showchar(3, 9, 'm');
		oled_showchar(3, 10, 'V');
		
		// 显示ADC值可以给个延迟，不然太快
		HAL_Delay(50);
    }
	else if (display_mode == 2)
	{
		oled_showstring(1, 6, "MPU6050");
		oled_showsignednum(2, 1, AX, 5);
		oled_showsignednum(3, 1, AY, 5);
		oled_showsignednum(4, 1, AZ, 5);
		oled_showsignednum(2, 8, GX, 5);
		oled_showsignednum(3, 8, GY, 5);
		oled_showsignednum(4, 8, GZ, 5);
	}
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
