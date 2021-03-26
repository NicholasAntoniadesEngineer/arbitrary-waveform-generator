/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
#include <string.h>
//#include "usbd_cdc_if.h" // this is needed to transmit a string of characters
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

/* Setting up signal generation */
int Res = 4096;				      // DAC resolution.
// NS 80 seems to work up to 25kHz
#define Ns 80 			          // Number of samples, Adjusting Ns will affect the frequency of the output signal.
// Buffer for all the sine bits.
uint32_t Channel_1_sine_val[Ns];
uint32_t Channel_2_sine_val[Ns];
// Scale value. Max value = sine_scaled*3.3. Will result in a deformed signal. Giving a max amplitude of 3.24V
double Channel_1_sine_scale = 0.7;
double Channel_2_sine_scale = 0.7;
int sine_dc_offset = 480; 	// DC off set value (4096Bits/3300mV)*200mV = 248.24Bits. Chec
#define PI 3.1415926		// Definition of PI
int Freq_Signal_1 = 5000; 	// Frequency of signal 1
int Freq_Signal_2 = 5000; 	// Frequency of signal 2
int PSC;					// Tim2 Pre Scalar value
uint32_t Fclock = 90000000;	// APB1 Timer Clocks
int Period = 1;				// Tim2 Period

void Get_channel_1_sine(void){
	// Fsine = FtimerRTGO/Ns,   Fsine = F(timer trigger ouput)/(number of samples)
	// Vsine(x)=(sine(x*(2PI/ns)+1)*((0xFFF+1)/2), this is an adjusted formula to create a positive sine.
	for(int i=0;i<Ns;i++){
		Channel_1_sine_val[i] = ((sin(i*2*PI/Ns)+1)*((Res)/2)); // Sampling step = 2PI/ns
		Channel_1_sine_val[i] = sine_dc_offset + Channel_1_sine_scale*Channel_1_sine_val[i];
	}
}

void Get_channel_2_sine(void){
	// Fsine = FtimerRTGO/Ns,   Fsine = F(timer trigger ouput)/(number of samples)
	// Vsine(x)=(sine(x*(2PI/ns)+1)*((0xFFF+1)/2), this is an adjusted formula to create a positive sine.
	for(int i=0;i<Ns;i++){
		Channel_2_sine_val[i] = ((sin(i*2*PI/Ns)+1)*((Res)/2)); // Sampling step = 2PI/ns
		Channel_2_sine_val[i] = sine_dc_offset + Channel_2_sine_scale*Channel_2_sine_val[i];
	}
}

void set_clock_TIM2(void){
	  // Fsine = FtimerRTGO/Ns,   Fsine = F(timer trigger ouput)/(number of samples)
	  // Adjust PSC and period in order to manipulate frequency.

	  PSC= (Fclock/Ns)/(Freq_Signal_1*(Period + 1) ) - 1;
	  htim2.Instance = TIM2;
	  htim2.Init.Period = Period; //+1
	  htim2.Init.Prescaler = PSC; //+1 // If this value is < 50 things start to behave funny.
	  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	  {
	    Error_Handler();
	  }
}

void set_clock_TIM4(void){
	  // Fsine = FtimerRTGO/Ns,   Fsine = F(timer trigger ouput)/(number of samples)
	  // Adjust PSC and period in order to manipulate frequency.

	  PSC= (Fclock/Ns)/(Freq_Signal_2*(Period + 1) ) - 1;
	  htim4.Instance = TIM4;
	  htim4.Init.Period = Period; //+1
	  htim4.Init.Prescaler = PSC; //+1 // If this value is < 50 things start to behave funny.
	  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
	  {
	    Error_Handler();
	  }
}


/* Setting up UART communications*/
#define uartSize_rx 8
#define uartSize_tx 40
uint8_t rx_buff[uartSize_rx];
uint8_t tx_buff[uartSize_tx];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

	uint32_t channel_1_Frequency;
	uint32_t channel_2_Frequency;
	uint32_t channel_1_Amplitude;
	uint32_t channel_2_Amplitude;

	// If statements to validate message integrity
	if((rx_buff[0] == '<') && (rx_buff[7] == '>')){
		//strcpy((char*)tx_buff, "Good message!\r\n");
		//HAL_UART_Transmit(&huart1, tx_buff, strlen((char*)tx_buff), HAL_MAX_DELAY);
		//HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART

		/*
		|<|(60/3C) : Start of message byte.
		|ADDR|() : Device Address byte.
		|CMD|() : Command byte.
		|DATA1| : Data byte 1.
		|DATA2| : Data byte 2.
		|DATA3| : Data byte 3.
		|DATA4| : Data byte 4.
		|>|(62/3E) : End of message byte.

		e.g. 3C 30 32 00 00 61 A8 3E, Setting channel 1 frequency to 25kHz
		e.g. 3C 30 33 00 00 61 A8 3E, Setting channel 2 frequency to 25kHz
		e.g. 3C 30 32 00 00 27 10 3E, Setting channel 1 frequency to 10kHz
		e.g. 3C 30 33 00 00 27 10 3E, Setting channel 2 frequency to 10kHz
		e.g. 3C 30 34 00 00 00 3C 3E, Setting channel 1 amplitude to 0.6
		e.g. 3C 30 35 00 00 00 3C 3E, Setting channel 2 amplitude to 0.6
		 */

		// Switch statements to respond accordingly
		switch(rx_buff[2]){

			// On/off command.
			// case 1:

			// Change the frequency of DAC channel 1.
			case '2':
				// Building 4 bytes int a 32 bit value
				channel_1_Frequency = rx_buff[6];
				channel_1_Frequency = channel_1_Frequency | (rx_buff[5] << 8);
				channel_1_Frequency = channel_1_Frequency | (rx_buff[4] << 16);
				channel_1_Frequency = channel_1_Frequency | (rx_buff[3] << 24);

				// Updating channel 1 output frequency
				Freq_Signal_1 = channel_1_Frequency;
				set_clock_TIM2();
				strcpy((char*)tx_buff, "Channel 1 Frequency updated: %d!\r\n", Freq_Signal_1);
				HAL_UART_Transmit(&huart1, tx_buff, strlen((char*)tx_buff), HAL_MAX_DELAY);
				HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART

				break;


			// Change the frequency of DAC channel 2.
			case '3':
				// Building 4 bytes int a 32 bit value
				channel_2_Frequency = rx_buff[6];
				channel_2_Frequency = channel_2_Frequency | (rx_buff[5] << 8);
				channel_2_Frequency = channel_2_Frequency | (rx_buff[4] << 16);
				channel_2_Frequency = channel_2_Frequency | (rx_buff[3] << 24);

				// Updating channel 1 output frequency
				Freq_Signal_2 = channel_2_Frequency;
				set_clock_TIM4();
				strcpy((char*)tx_buff, "Channel 2 Frequency updated: %d!\r\n", Freq_Signal_2);
				HAL_UART_Transmit(&huart1, tx_buff, strlen((char*)tx_buff), HAL_MAX_DELAY);
				HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART

				break;


			// Change the amplitude of DAC channel 1.
			case '4':
				// Building 4 bytes int a 32 bit value
				channel_1_Amplitude = rx_buff[6];
				channel_1_Amplitude = channel_1_Amplitude | (rx_buff[5] << 8);
				channel_1_Amplitude = channel_1_Amplitude | (rx_buff[4] << 16);
				channel_1_Amplitude = channel_1_Amplitude | (rx_buff[3] << 24);

				// Updating channel 1 amplitude
				Channel_1_sine_scale = (double)channel_1_Amplitude/100; // Dividing by 100 to create a fraction
				Get_channel_1_sine();
				HAL_DAC_Start_DMA(&hdac, DAC1_CHANNEL_1, Channel_1_sine_val, Ns, DAC_ALIGN_12B_R); //Start DMA, passing list of sine values.
				strcpy((char*)tx_buff, "Channel 1 Amplitude updated: %d!\r\n", Channel_1_sine_scale);
				HAL_UART_Transmit(&huart1, tx_buff, strlen((char*)tx_buff), HAL_MAX_DELAY);
				HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART

				break;


			// Change the amplitude of DAC channel 2.
			case '5':
				// Building 4 bytes int a 32 bit value
				channel_2_Amplitude = rx_buff[6];
				channel_2_Amplitude = channel_2_Amplitude | (rx_buff[5] << 8);
				channel_2_Amplitude = channel_2_Amplitude | (rx_buff[4] << 16);
				channel_2_Amplitude = channel_2_Amplitude | (rx_buff[3] << 24);

				// Updating channel 2 amplitude
				Channel_2_sine_scale = (double)channel_2_Amplitude/100; // Dividing by 100 to create a fraction
				Get_channel_2_sine();
				HAL_DAC_Start_DMA(&hdac, DAC1_CHANNEL_2, Channel_2_sine_val, Ns, DAC_ALIGN_12B_R); //Start DMA, passing list of sine values.
				strcpy((char*)tx_buff, "Channel 2 Amplitude updated: %d!\r\n", Channel_2_sine_scale);
				HAL_UART_Transmit(&huart1, tx_buff, strlen((char*)tx_buff), HAL_MAX_DELAY);
				HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART

				break;


			// Request Voltage and Current measurement of channel 1 output.
			case 6:
				break;


			// Request Voltage and Current measurement of channel 2 output.
			case 7:
				break;


			// Request Temperature sensor 1 and 2 output.
			case 8:
				break;


			// Acknowledge message received.
			case 9:
				break;


			// Bad message received.
			case 10:
				break;


			// Request current system state.
			case 11:
				break;
		}


	}

}

void HAL_UART_TxCpltCallback (UART_HandleTypeDef *huart){
//	int i = 100;
//	while(i>0){
//		i = i -1;
//	}
	HAL_UART_DMAPause(&huart1);
	HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART
}

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_DAC_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  /* Sine function */
  set_clock_TIM2();						// Setting frequency of timer 2
  set_clock_TIM4();						// Setting frequency of timer 4
  HAL_TIM_Base_Start(&htim2);			// Start timer 2
  HAL_TIM_Base_Start(&htim4);			// Start timer 4

  Get_channel_1_sine();
  HAL_DAC_Start_DMA(&hdac, DAC1_CHANNEL_1, Channel_1_sine_val, Ns, DAC_ALIGN_12B_R); //Start DMA, passing list of sine values.

  Get_channel_2_sine();
  HAL_DAC_Start_DMA(&hdac, DAC1_CHANNEL_2, Channel_2_sine_val, Ns, DAC_ALIGN_12B_R); //Start DMA, passing list of sine values.



  /* Setting signal output indicators to on  */
  HAL_GPIO_WritePin(GPIOD, LED1_Pin, 1);
  HAL_GPIO_WritePin(GPIOD, LED2_Pin, 1);

  /* Setting up UART communications */
  HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize_rx); // Receive UART

  /* Setting up USB communications*/
  //  char txBuf[8];
  //  uint8_t count = 1;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	//HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize); //set correct UART handler
	HAL_Delay(1000);



	// UART DMA message
	// HAL_UART_Receive_DMA(&huart1, rx_buff, uartSize); // Receive UART
	// HAL_UART_DMAResume(&huart1);
	// HAL_UART_Transmit_DMA(&huart1, tx_buff, strlen((char*)tx_buff));


	/* LED TEST */
	//HAL_GPIO_TogglePin(GPIOD, LED3_Pin);
	//HAL_GPIO_TogglePin(GPIOD, LED4_Pin);
	//HAL_GPIO_TogglePin(GPIOD, LED5_Pin);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
