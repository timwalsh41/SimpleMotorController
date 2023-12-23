/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SingletonFactory.h"
#include "SerialParser.h"
#include "IOutputGateway.h"
#include "OutputGateway.h"
#include "MessageHandler.h"
#include "CurrentSensor.h"
#include "Controller.h"
#include "IMotorDriver.h"
#include "PololuG2MotorDriver.h"
#include "MotorCommandHandler.h"
#include "STM32Timebase.h"
#include "IUartWrapper.h"
#include "StmUartWrapper.h"
#include "Version.h"
#include "Utility.h"
#include "BaseLogger.h"
#include "RealLogger.h"
#include <memory>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

using MessageHandling::MessageHandler;
using MessageHandling::SerialParser;
using MessageHandling::IOutputGateway;
using MessageHandling::OutputGateway;
using Sensors::CurrentSensor;
using MotorControl::Controller;
using MotorControl::IMotorCommandHandler;
using MotorControl::MotorCommandHandler;
using MotorControl::IMotorDriver;
using MotorControl::PololuG2MotorDriver;
using Logging::RealLogger;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
constexpr uint32_t MAIN_LOOP_TIME_MS = 1;
constexpr uint32_t LED_BLINK_TIME_MS = 500;
constexpr uint32_t HEARTBEAT_TIME_MS = 5000;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */
uint8_t UartRcvBuffer[UART_RECEIVE_BUFFER_SIZE];
int32_t UartDmaIdx = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
static void processUartDma();
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
  uint32_t mainLoopTimer = 0;
  uint32_t ledTimer = 0;
  uint32_t heartbeatTimer = 0;
  std::string heartbeatMsg = "Heartbeat";

  SingletonFactory factory;

  std::shared_ptr<ITimebase> pTimebase = factory.create<ITimebase, STM32Timebase>();

  // create UART wrapper around uart2 interface
  StmUartWrapper uartWrapper(&huart2);

  // create the motor driver (does not use logging)
  PololuG2MotorDriver theMotorDriver;

  // create the logging singleton as that is accessed by all other objects
  std::shared_ptr<Logging::BaseLogger> pLogger = factory.create<Logging::BaseLogger, RealLogger>();

  // Output gateway only requires the logger, and then we need to load the gateway into the logger
  // (because there is a circular dependency here, we create the logger, then the gateway, then register
  // the gateway with the logger)
  std::shared_ptr<IOutputGateway> pOutputGateway = factory.create<IOutputGateway, OutputGateway>(static_cast<IUartWrapper*>(&uartWrapper));
  std::static_pointer_cast<RealLogger>(pLogger)->loadOutputGateway();

  // Current sensor and controller also only require the logger
  std::shared_ptr<CurrentSensor> pCurrentSensor = factory.create<CurrentSensor>();
  std::shared_ptr<Controller> pController = factory.create<Controller>(static_cast<IMotorDriver*>(&theMotorDriver));

  // command handler requires logger and controller
  std::shared_ptr<IMotorCommandHandler> pMotorCommandHandler = factory.create<IMotorCommandHandler, MotorCommandHandler>(pController);

  // Message handler requires everything (logger, gateway, controller, current sensor, motor command handler)
  MessageHandler theMessageHandler;

  // serial parser requires logger and message handler
  std::shared_ptr<SerialParser> pSerialParser = factory.create<SerialParser>(&theMessageHandler);

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
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  // now start TIM2 for PWM generation and ADC triggering
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

  // start ADC acquisitions
  HAL_ADC_Start_DMA(&hadc1, pCurrentSensor->getCurrentBuffer(), Sensors::CURRENT_SENSOR_BUFFER_SIZE);

  // set the UART in receive mode (with DMA)
  HAL_UART_Receive_DMA(&huart2, UartRcvBuffer, UART_RECEIVE_BUFFER_SIZE);

  // start TIM1 in quadrature encoder mode
  if (HAL_TIM_Encoder_Start_IT(&htim1, TIM_CHANNEL_ALL) != HAL_OK)
  		Error_Handler();

  // apply offset to zero encoder mid-range
  theMotorDriver.zeroEncoder();

  // give a short delay to build up current samples, then we'll zero the sensor
  HAL_Delay(100);
  pCurrentSensor->zeroCurrentSensor();

#ifdef DEBUG
  uint8_t version[3];
  Utility::getVersion(version);
  std::string welcomeString = "Welcome to the hammock driver v" + \
		  	  	  	  	  	  std::to_string(version[0]) + "." + \
							  std::to_string(version[1]) + "." + \
							  std::to_string(version[2]);
  pLogger->logString(info, welcomeString);
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	if (HAL_GetTick() < mainLoopTimer)
		// tick rolled over, reset timer
		mainLoopTimer = 0;

	if ((HAL_GetTick() - mainLoopTimer) >= MAIN_LOOP_TIME_MS)
	{
		// Main loop tasks
		processUartDma();
		pSerialParser->ProcessIncomingData();
		theMessageHandler.processMessages();
		pOutputGateway->processMessages();
		pCurrentSensor->processCurrentMeasurements();
		pController->tick();

		mainLoopTimer = HAL_GetTick();
	}

	if (Utility::timeElapsed(ledTimer, HAL_GetTick()) >= LED_BLINK_TIME_MS)
	{
		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		ledTimer = HAL_GetTick();
	}

	if (Utility::timeElapsed(heartbeatTimer, HAL_GetTick()) >= HEARTBEAT_TIME_MS)
	{
		pLogger->logString(debug, heartbeatMsg);
		heartbeatTimer = HAL_GetTick();
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 5;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T2_TRGO;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1600;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC2REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = 10;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 921600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DIR_Pin|LD3_Pin|SLP__Pin|FLAG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DIR_Pin LD3_Pin SLP__Pin FLAG_Pin */
  GPIO_InitStruct.Pin = DIR_Pin|LD3_Pin|SLP__Pin|FLAG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FLT__Pin */
  GPIO_InitStruct.Pin = FLT__Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(FLT__GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// SingletonFactory factory;
	// auto pSerialParser = factory.retrieve<SerialParser>();
	// pSerialParser->ReceiveChar(UartRcvBuffer);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	SingletonFactory factory;
	auto pOutputGateway = factory.retrieve<IOutputGateway>();

	HAL_UART_Receive_DMA(&huart2, UartRcvBuffer, UART_RECEIVE_BUFFER_SIZE);
	std::static_pointer_cast<OutputGateway>(pOutputGateway)->setReceiving();
}

// Called when ADC DMA buffer is completely filled
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  SingletonFactory factory;
  auto pCurrentSensor = factory.retrieve<CurrentSensor>();

  HAL_ADC_Stop_DMA(&hadc1);

  // hand off current sensor buffer
  pCurrentSensor->swapBuffer();

  // restart the measurement with the new buffer
  HAL_ADC_Start_DMA(&hadc1, pCurrentSensor->getCurrentBuffer(), Sensors::CURRENT_SENSOR_BUFFER_SIZE);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	SingletonFactory factory;
	auto pController = factory.retrieve<Controller>();

	if (GPIO_Pin == FLT__Pin)
		if (HAL_GPIO_ReadPin(FLT__GPIO_Port, FLT__Pin) == GPIO_PIN_RESET)
			pController->injectFault();
}

void processUartDma()
{
	SingletonFactory factory;
	auto pSerialParser = factory.retrieve<SerialParser>();
	std::vector<uint8_t> newChars{};

	// Note - I had this as an atomic operation (disabling interrupts and pausing the UART/DMA transfer
	// However this was leading to missed bytes - at a baud rate of 921,600, we get a byte almost every
	// microsecond, so if a transmission arrives during this operation we will quickly fill the HW UART
	// buffer and start losing data. Once I removed the disable IRQ and DMA pause, transmission was much
	// more reliable. Checking timing using the GPIO flag, this would take about 16 us when I caught an
	// instance that involved a DMA read while running with MotorCommander script (sends a few commands
	// every tick to get status and sensor values).

	// __disable_irq();
	// HAL_UART_DMAPause(&huart2);
	int32_t new_pos = UART_RECEIVE_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart2.hdmarx);

	if (new_pos != UartDmaIdx) { // New data has arrived; potential bug here if exactly UART_RECEIVE_BUFFER_SIZE
								 // bytes arrive
		if (new_pos > UartDmaIdx) { // No overflow
			// Process data from buffer[old_pos] to buffer[new_pos-1]
			newChars.insert(newChars.end(), &UartRcvBuffer[UartDmaIdx], &UartRcvBuffer[new_pos]);
		} else { // Overflow
			// First process data from buffer[old_pos] to buffer[BUFFER_SIZE-1]
			newChars.insert(newChars.end(), &UartRcvBuffer[UartDmaIdx], &UartRcvBuffer[UART_RECEIVE_BUFFER_SIZE]);

			// Then process data from buffer[0] to buffer[new_pos-1]
			newChars.insert(newChars.end(), &UartRcvBuffer[0], &UartRcvBuffer[new_pos]);
		}

		UartDmaIdx = new_pos;
		pSerialParser->ReceiveChars(newChars);
	}

	// HAL_UART_DMAResume(&huart2);
	// __enable_irq();
	// end atomic operation
}

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
