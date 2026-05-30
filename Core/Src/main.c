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
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "openx07v_c_lcd.h"
#include "touch_panel.h"
#include "arm_math.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* FFT settings */
#define SAMPLES					512			/* 256 real party and 256 imaginary parts */
#define FFT_SIZE				SAMPLES / 2		/* FFT size is always the same size as we have samples, so 256 in our case */

/* Global variables */
float32_t Input[SAMPLES];
float32_t Output[FFT_SIZE];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern uint8_t tim7_125us_flag;

uint8_t AdcVal = 0;
uint8_t Adc_Array[320];

#if ENABLE_UART_MONITOR
uint8_t Tx_Message[1029];
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void LCD_TFT_Init_Calib(void);
#if ENABLE_UART_MONITOR
void Encode_Message_Raw(float32_t* RawData_array, uint16_t Raw_Message_Length);
void Encode_Message_FFT_f32(float32_t* FFT_OutArray, uint16_t FFT_Message_Length);
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint16_t IndexCounter = 0;
	uint8_t ModeState = 0;
	uint16_t TimeDomainIndex = 0;

	arm_cfft_radix4_instance_f32 S;	/* ARM CFFT module */
	uint16_t i;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	for(uint16_t j = 0; j < 320; j++)
	{
		Adc_Array[j] = 0;
	}
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_FSMC_Init();
  MX_TIM7_Init();
  MX_USART3_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Transmit(&huart3, (uint8_t*)"USART3 OK\r\n", 11, 100);

	BSP_LCD_Init();
	LCD_TFT_Init_Calib();
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&AdcVal, 1);
	HAL_TIM_Base_Start_IT(&htim7);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(ModeState == 0)
		{
			if(tim7_125us_flag == 1)
			{
				tim7_125us_flag = 0;
				memmove(Input+2, Input, sizeof(Input) - 2*sizeof(float32_t));
				Input[0] = (AdcVal/4);
				Input[1] = 0;

				BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
				BSP_LCD_FillCircle(TimeDomainIndex, Input[0]+40, 1);

				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				BSP_LCD_DrawVLine(TimeDomainIndex+1, 35, 70);

				TimeDomainIndex ++;
				IndexCounter += 2;

				if(TimeDomainIndex == 320)
				{
					TimeDomainIndex = 0;
				}

				if(IndexCounter == SAMPLES)
				{
					IndexCounter = 0;
					BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
					for(i = 2; i < (FFT_SIZE/2); i++)
					{
						BSP_LCD_DrawLine(i, 240, i, 240-(Output[i]/25));
					}


					arm_cfft_radix4_init_f32(&S, FFT_SIZE, 0, 1);
					arm_cfft_radix4_f32(&S, Input);
					arm_cmplx_mag_f32(Input, Output, FFT_SIZE);

#if ENABLE_UART_MONITOR
					Encode_Message_Raw(Input, SAMPLES);
					HAL_UART_Transmit(&huart3, &Tx_Message[0], (SAMPLES/2) + 5, HAL_MAX_DELAY);

					Encode_Message_FFT_f32(Output, FFT_SIZE);
					HAL_UART_Transmit(&huart3, &Tx_Message[0], (FFT_SIZE * 4) + 5, HAL_MAX_DELAY);
#endif

					BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
					for(i = 2; i < (FFT_SIZE/2); i++)
					{
						BSP_LCD_DrawLine(i, 240, i, 240-(Output[i]/25));
					}
				}
			}
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
void LCD_TFT_Init_Calib(void)
{
	BSP_LCD_DisplayOn();

	TouchPanel_Calibrate();
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_Clear(LCD_COLOR_BLACK);

	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_FillRect(0, 0, 120, 17);
	BSP_LCD_FillRect(255, 0, 65, 17);

	BSP_LCD_SetFont(&Font12);
	BSP_LCD_SetBackColor(LCD_COLOR_LIGHTGRAY);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(3,3, (uint8_t*)"Time Domain View",LEFT_MODE);
	BSP_LCD_DisplayStringAt(0,3, (uint8_t*)"FFT View",RIGHT_MODE);
}

#if ENABLE_UART_MONITOR
/**
 * @brief Build a "raw" frame in Tx_Message for the C# monitor.
 *        Reads even-indexed elements of RawData_array (which holds
 *        AdcVal/4 at [0], 0 at [1], AdcVal_prev/4 at [2], ... — the
 *        CFFT real/imag interleave). Sends those as 1-byte samples.
 *        Output: 5-byte header + (Raw_Message_Length/2) data bytes.
 */
void Encode_Message_Raw(float32_t* RawData_array, uint16_t Raw_Message_Length)
{
	uint16_t i;
	uint16_t Counter = 0;

	Tx_Message[0] = 0x03;
	Tx_Message[1] = 0x15;
	Tx_Message[2] = 0x01;
	Tx_Message[3] = ((Raw_Message_Length / 2) >> 8) & 0xFF;
	Tx_Message[4] = (Raw_Message_Length / 2) & 0xFF;

	for (i = 0; i < Raw_Message_Length; i++)
	{
		if ((i % 2) == 0)
		{
			Tx_Message[Counter + 5] = (uint8_t)RawData_array[i];
			Counter++;
		}
	}
}

void Encode_Message_FFT_f32(float32_t* FFT_OutArray, uint16_t FFT_Message_Length)
{
	uint16_t i;
	uint16_t Counter = 0;
	unsigned char ch[4];

	Tx_Message[0] = 0x03;
	Tx_Message[1] = 0x15;
	Tx_Message[2] = 0x02;
	Tx_Message[3] = ((FFT_Message_Length * 4) >> 8) & 0xFF;
	Tx_Message[4] = (FFT_Message_Length * 4) & 0xFF;

	for (i = 0; i < FFT_Message_Length; i++)
	{
		memcpy(ch, &FFT_OutArray[i], sizeof(float));
		for (int j = 0; j < 4; j++)
		{
			Tx_Message[Counter + 5] = ch[j];
			Counter++;
		}
	}
}
#endif
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
