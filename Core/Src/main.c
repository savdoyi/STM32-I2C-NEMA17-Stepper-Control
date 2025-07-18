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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint8_t data_length = 0;    // 1-byte -- data length
uint8_t received_data[MAX_DATA_SIZE];  // buffer for data
int a[MAX_DATA_SIZE];       // Int massivi, ESP32 dan kelgan ma'lumotlar
uint8_t receive_step = 0;   // receive step (0 - length waiting, 1 - data waiting)
uint8_t receive_flag = 0;   // data is ready flag

// Global o'zgaruvchilar - TIM1 va TIM2 interrupt handlerlarida ishlatiladi
int tim1_counter_1 = 0; // Motor 1 uchun qadam sanagich (ishlayotgan vaqtda qolgan pulslar)
int tim2_counter_1 = 0; // Motor 2 uchun qadam sanagich (ishlayotgan vaqtda qolgan pulslar)

// YANGI GLOBAL O'ZGARUVCHILAR - HAR BIR MOTOR UCHUN KONFIGURATSIYA VA PULSLARNI SAQLASH UCHUN
uint8_t motor1_config[7] = {0}; // mode, id, enable, dir, velocity, byte_5, byte_6
uint8_t motor2_config[7] = {0}; // mode, id, enable, dir, velocity, byte_5, byte_6

volatile int motor1_total_pulses = 0; // Motor 1 uchun hisoblangan umumiy pulslar soni
volatile int motor2_total_pulses = 0; // Motor 2 uchun hisoblangan umumiy pulslar soni

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
// I2C callback funksiyalarini bu yerda e'lon qilish
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode);
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c);
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_I2C_EnableListen_IT(&hi2c1); // I2C ni boshida tinglash rejimiga o'rnatish

  // Motor 1 pinlarini boshlang'ich holatga keltirish
  // Drayver Enable pini PAST (0) da faol bo'lsa, SET = o'chirilgan holat (Disabled)
  HAL_GPIO_WritePin(Enable_1_GPIO_Port, Enable_1_Pin, SET); // Motor drayverini boshida o'chirish (Disabled)
  HAL_GPIO_WritePin(Direction_1_GPIO_Port, Direction_1_Pin, RESET); // Yo'nalishni RESET holatiga qo'yish (drayver talabiga qarab)
  HAL_GPIO_WritePin(Pulse_1_GPIO_Port, Pulse_1_Pin, RESET); // Puls pinini RESET holatiga qo'yish

  // Motor 2 pinlarini boshlang'ich holatga keltirish
  // Drayver Enable pini PAST (0) da faol bo'lsa, SET = o'chirilgan holat (Disabled)
  HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, SET); // Motor drayverini boshida o'chirish (Disabled)
  HAL_GPIO_WritePin(Direction_2_GPIO_Port, Direction_2_Pin, RESET); // Yo'nalishni RESET holatiga qo'yish (drayver talabiga qarab)
  HAL_GPIO_WritePin(Pulse_2_GPIO_Port, Pulse_2_Pin, RESET); // Puls pinini RESET holatiga qo'yish
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(receive_flag)
	  {
		  receive_flag = 0;

		  // I2C ma'lumotlar formati:
		  // a[0] = mode (0 - only config, 1 - start or config+start, 2 - stop)
		  // a[1] = motor id (1 - Motor 1, 0 - Motor 2)
		  // a[2] = Enable (0=disabled, 1=enabled)
		  // a[3] = Direction (0=clockwise, 1=counter-clockwise)
		  // a[4] = Velocity (gradus/sekund)
		  // a[5] = degree_byte_high (darajaning yuzlik va mingliklari)
		  // a[6] = degree_byte_low (darajaning birlik va o'nliklari)
		  // Jami gradus = a[5]*100 + a[6]

		  // Stop buyrug'i (a[0] == 2) - barcha motorlarni to'xtatish
		  if(a[0] == 2)
		  {
		      HAL_TIM_Base_Stop_IT(&htim1);
		      HAL_GPIO_WritePin(Enable_1_GPIO_Port, Enable_1_Pin, SET); // Motor 1 drayverini o'chirish
		      tim1_counter_1 = 0; // Hisoblagichni nolga tiklash

		      HAL_TIM_Base_Stop_IT(&htim2);
		      HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, SET); // Motor 2 drayverini o'chirish
		      tim2_counter_1 = 0; // Hisoblagichni nolga tiklash
		  }
		  else if(a[0] == 0) // Faqat Konfiguratsiya buyrug'i (a[0] == 0)
		  {
			  // Motor 1 uchun buyruq
			  if(a[1] == 1)
			  {
				  // Konfiguratsiya parametrlarini motor1_config massiviga nusxalash
				  for(int i = 0; i < data_length && i < 7; i++) {
				      motor1_config[i] = a[i];
				  }

				  if(motor1_config[2] == 0)	// Motor 1 disabled (Enable_1_Pin = SET)
				  {
					  HAL_GPIO_WritePin(Enable_1_GPIO_Port, Enable_1_Pin, SET); // Motor drayverini o'chirish (Disabled)
					  HAL_TIM_Base_Stop_IT(&htim1); // Taymerni to'xtatish
					  tim1_counter_1 = 0; // Qadam sanagichni nolga qaytarish
					  motor1_total_pulses = 0; // Pulslarni ham nolga tiklash
				  }
				  else		// Motor 1 enabled (Enable_1_Pin = RESET)
				  {
					  HAL_GPIO_WritePin(Enable_1_GPIO_Port, Enable_1_Pin, RESET); // Motor drayverini yoqish (Enabled)

					  if(motor1_config[3] == 0) // Direction: Clockwise (Drayver mantiqiga qarab SET/RESET)
					  {
						  HAL_GPIO_WritePin(Direction_1_GPIO_Port, Direction_1_Pin, SET);
					  }
					  else		// Direction: Counter-clockwise (Drayver mantiqiga qarab SET/RESET)
					  {
						  HAL_GPIO_WritePin(Direction_1_GPIO_Port, Direction_1_Pin, RESET);
					  }

					  // Pulslar sonini hisoblash (daraja asosida) va saqlash
					  int current_total_degree = motor1_config[5] * 100 + motor1_config[6];
					  if (MS_4 == 0) { /* MS_4 nol bo'lsa xato holatini boshqaring */ }
					  motor1_total_pulses = (int)(((float)current_total_degree * (float)MS_4 / 360.0f));
					  motor1_total_pulses *= 2; // Har bir qadam uchun ikki marta toggle qilish (HIGH va LOW uchun)


					  // Tezlikni sozlash (Prescaler) - BU QISM TUZATILDI
					  HAL_TIM_Base_Stop_IT(&htim1); // Taymerni to'xtatish

					  if (motor1_config[4] > 0) // gradus/sekund > 0 bo'lsa
					  {
						  float gradus_per_second_m1 = (float)motor1_config[4];
						  float steps_per_second_m1 = (gradus_per_second_m1 / 360.0f) * MS_4; // MS_4 = 800
						  float toggle_operations_per_second_m1 = steps_per_second_m1 * 2.0f;

						  if (toggle_operations_per_second_m1 > 0)
						  {
							  // Prescaler = (Takt chastotasi / (Kerakli_Toggle_Chastotasi * (Period + 1))) - 1
							  // SystemCoreClock = 72MHz, Period = 49 (ya'ni Period + 1 = 50)
							  int new_ps = (int)((float)SystemCoreClock / (toggle_operations_per_second_m1 * (htim1.Init.Period + 1))) - 1;
							  if (new_ps < 0) new_ps = 0;
							  htim1.Init.Prescaler = (uint16_t)new_ps;
						  }
						  else
						  {
							  // Agar hisoblangan toggle chastotasi nol yoki manfiy bo'lsa
							  HAL_TIM_Base_Stop_IT(&htim1);
							  tim1_counter_1 = 0;
							  motor1_total_pulses = 0;
							  htim1.Init.Prescaler = 7199;
						  }
					  }
					  else // motor1_config[4] == 0 bo'lsa, motorni to'xtatish
					  {
						  HAL_TIM_Base_Stop_IT(&htim1);
						  tim1_counter_1 = 0; // Qadam sanagichni ham 0 qilish
						  motor1_total_pulses = 0; // Pulslarni ham 0 qilish
						  htim1.Init.Prescaler = 7199; // Standart yoki maksimal prescaler
					  }

					  // YANGI PRESCALER BILAN TAYMERNI QAYTA INITSIALIZATSIYA QILISH! BU JUDA MUHIM!
					  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
					  {
						  Error_Handler(); // Taymerni initsializatsiya qilishda xato yuzaga kelsa
					  }
				  }
			  }
			  // Motor 2 uchun buyruq (TIM2 orqali)
			  else if(a[1] == 0) // Motor ID 0 deb berilgan, Motor 2 uchun
			  {
				  // Konfiguratsiya parametrlarini motor2_config massiviga nusxalash
				  for(int i = 0; i < data_length && i < 7; i++) {
				      motor2_config[i] = a[i];
				  }

				  if(motor2_config[2] == 0)	// Motor 2 disabled (Enable_2_Pin = SET)
				  {
					  HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, SET); // Motor drayverini o'chirish
					  HAL_TIM_Base_Stop_IT(&htim2); // TIM2 ni to'xtatish
					  tim2_counter_1 = 0; // Qadam sanagichni nolga qaytarish
					  motor2_total_pulses = 0; // Pulslarni ham nolga tiklash
				  }
				  else		// Motor 2 enabled (Enable_2_Pin = RESET)
				  {
					  HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, RESET); // Motor drayverini yoqish

					  if(motor2_config[3] == 0) // Direction: Clockwise (Drayver mantiqiga qarab SET/RESET)
					  {
						  HAL_GPIO_WritePin(Direction_2_GPIO_Port, Direction_2_Pin, SET);
					  }
					  else		// Direction: Counter-clockwise (Drayver mantiqiga qarab SET/RESET)
					  {
						  HAL_GPIO_WritePin(Direction_2_GPIO_Port, Direction_2_Pin, RESET);
					  }

					  // Pulslar sonini hisoblash (daraja asosida) va saqlash
					  int current_total_degree = motor2_config[5] * 100 + motor2_config[6];
					  if (MS_4 == 0) { /* MS_4 nol bo'lsa xato holatini boshqaring */ }
					  motor2_total_pulses = (int)(((float)current_total_degree * (float)MS_4 / 360.0f));
					  motor2_total_pulses *= 2; // Har bir qadam uchun ikki marta toggle qilish (HIGH va LOW uchun)

					  // Tezlikni sozlash (Prescaler) - BU QISM TUZATILDI
					  HAL_TIM_Base_Stop_IT(&htim2); // TIM2 ni to'xtatish

					  if (motor2_config[4] > 0) // gradus/sekund > 0 bo'lsa
					  {
						  float gradus_per_second_m2 = (float)motor2_config[4];
						  float steps_per_second_m2 = (gradus_per_second_m2 / 360.0f) * MS_4;
						  float toggle_operations_per_second_m2 = steps_per_second_m2 * 2.0f;

						  if (toggle_operations_per_second_m2 > 0)
						  {
							  int new_ps = (int)((float)SystemCoreClock / (toggle_operations_per_second_m2 * (htim2.Init.Period + 1))) - 1;
							  if (new_ps < 0) new_ps = 0;
							  htim2.Init.Prescaler = (uint16_t)new_ps;
						  }
						  else
						  {
							  // Agar hisoblangan toggle chastotasi nol yoki manfiy bo'lsa
							  HAL_TIM_Base_Stop_IT(&htim2);
							  tim2_counter_1 = 0;
							  motor2_total_pulses = 0;
							  htim2.Init.Prescaler = 7199;
						  }
					  }
					  else // motor2_config[4] == 0 bo'lsa, motorni to'xtatish
					  {
						  HAL_TIM_Base_Stop_IT(&htim2);
						  tim2_counter_1 = 0; // Qadam sanagichni ham 0 qilish
						  motor2_total_pulses = 0; // Pulslarni ham 0 qilish
						  htim2.Init.Prescaler = 7199; // Standart yoki maksimal prescaler
					  }

					  // YANGI PRESCALER BILAN TAYMERNI QAYTA INITSIALIZATSIYA QILISH! BU JUDA MUHIM!
					  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) // htim2 ni qayta initsializatsiya qilish
					  {
						  Error_Handler(); // Taymerni initsializatsiya qilishda xato yuzaga kelsa
					  }
				  }
			  }
		  }
		  else if(a[0] == 1) // Start buyrug'i (a[0] == 1) - Ikkala motorni ham ishga tushirish
		  {
			  // Motor 1 ni ishga tushirish, agar u ilgari konfiguratsiya qilingan va enabled bo'lsa
			  // va harakatlanish uchun pulslar mavjud bo'lsa
			  if(motor1_config[2] == 1 && motor1_config[4] > 0 && motor1_total_pulses > 0)
			  {
				  // Motor 1 uchun yo'nalishni qayta sozlash (faqat startda yangilanishi uchun)
				  if(motor1_config[3] == 0) // CW
				  {
					  HAL_GPIO_WritePin(Direction_1_GPIO_Port, Direction_1_Pin, SET);
				  }
				  else		// CCW
				  {
					  HAL_GPIO_WritePin(Direction_1_GPIO_Port, Direction_1_Pin, RESET);
				  }

				  tim1_counter_1 = motor1_total_pulses; // Hisoblangan pulslarni o'rnatish
				  HAL_TIM_Base_Start_IT(&htim1); // Taymer interruptini boshlash
			  }

			  // Motor 2 ni ishga tushirish, agar u ilgari konfiguratsiya qilingan va enabled bo'lsa
			  // va harakatlanish uchun pulslar mavjud bo'lsa
			  if(motor2_config[2] == 1 && motor2_config[4] > 0 && motor2_total_pulses > 0)
			  {
				  // Motor 2 uchun yo'nalishni qayta sozlash (faqat startda yangilanishi uchun)
				  if(motor2_config[3] == 0) // CW
				  {
					  HAL_GPIO_WritePin(Direction_2_GPIO_Port, Direction_2_Pin, SET);
				  }
				  else		// CCW
				  {
					  HAL_GPIO_WritePin(Direction_2_GPIO_Port, Direction_2_Pin, RESET);
				  }

				  tim2_counter_1 = motor2_total_pulses; // Hisoblangan pulslarni o'rnatish
				  HAL_TIM_Base_Start_IT(&htim2); // Taymer interruptini boshlash
			  }
		  }
    	}
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 64; // !!! Avvalgi suhbatlarimizdagi natija: 0x20 ESP32 manziliga mos keladi
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7199; // Dastlabki qiymat, ish vaqtida o'zgaradi
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 49;     // 50% ish sikli uchun (N-1) formula: 50 - 1 = 49
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // Taymerning UPDATE hodisasida trigger
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

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 49;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // !!! MUHIM: TIM_TRGO_RESET emas, TIM_TRGO_UPDATE !!!
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  // __HAL_RCC_GPIOD_CLK_ENABLE(); // Agar PD portida GPIO ishlatilmayotgan bo'lsa, o'chirishingiz mumkin.
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level for PC13 (if used as LED, etc.) */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level for Motor 1 */
  // Agar Enable_1_Pin va Direction_1_Pin PC portida bo'lsa:
  HAL_GPIO_WritePin(Enable_1_GPIO_Port, Enable_1_Pin | Direction_1_Pin, GPIO_PIN_RESET); // Qanday holatda boshlanishi sizning loyihangizga bog'liq
  HAL_GPIO_WritePin(Pulse_1_GPIO_Port, Pulse_1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level for Motor 2 */
  HAL_GPIO_WritePin(GPIOB, Enable_2_Pin | Direction_2_Pin | Pulse_2_Pin, GPIO_PIN_RESET); // Dastlabki holatni belgilash

  /*Configure GPIO pins : PC13 (example for a general purpose pin like LED) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins for Motor 1 */
  GPIO_InitStruct.Pin = Enable_1_Pin | Direction_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Enable_1_GPIO_Port, &GPIO_InitStruct); // Enable_1, Direction_1 pinlari uchun

  // Pulse_1_Pin alohida portda bo'lgani uchun alohida sozlash
  GPIO_InitStruct.Pin = Pulse_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Pulse_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins for Motor 2 (PB.12, PB.13, PB.14) */
  GPIO_InitStruct.Pin = Enable_2_Pin | Direction_2_Pin | Pulse_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); // Barcha motor 2 pinlari (PB12, PB13, PB14)

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  I2C Master listens for end of transfer.
  * @param  hi2c: I2C handle
  * @retval None
  */
void HAL_I2C_ListenCpltCallback (I2C_HandleTypeDef *hi2c)
{
    // Tinglash tsikli tugaganda (masalan, I2C aloqasi tugaganda) yana tinglashni yoqish
    HAL_I2C_EnableListen_IT(hi2c);
}

/**
  * @brief  I2C Master transmits data.
  * @param  hi2c: I2C handle
  * @param  TransferDirection: I2C_DIRECTION_TRANSMIT or I2C_DIRECTION_RECEIVE
  * @param  AddrMatchCode: matched address
  * @retval None
  */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	 if (TransferDirection == I2C_DIRECTION_TRANSMIT) // Masterdan yozish
	    {
	        if(receive_step == 0) {
	        	// Birinchi bayt - data uzunligi
	        	// Bu birinchi frame bo'lgani uchun I2C_FIRST_FRAME ishlatamiz
	        	HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &data_length, 1, I2C_FIRST_FRAME);
	        }
	        else {
	            // Qolgan ma'lumotlar
	            // Bu oxirgi frame bo'lgani uchun I2C_LAST_FRAME ishlatamiz
	            HAL_I2C_Slave_Sequential_Receive_IT(hi2c, received_data, data_length, I2C_LAST_FRAME);
	        }
	    }
}

/**
  * @brief  Slave Receive complete callback.
  * @param  hi2c: I2C handle
  * @retval None
  */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if(receive_step == 0) {
		receive_step = 1; // Keyingi qadamga o'tish: ma'lumotlarni qabul qilish
		if(data_length > MAX_DATA_SIZE) {
			data_length = MAX_DATA_SIZE; // Buffer to'lib ketishini oldini olish
		}
        // Ma'lumot uzunligi qabul qilindi, endi ma'lumotlarni kutish uchun qayta tinglash
        HAL_I2C_EnableListen_IT(hi2c); // I2C_FIRST_FRAME dan keyin tinglashni davom ettirish
	}
	else {
		// Ma'lumotlar to'liq qabul qilindi
		for(int i = 0; i < data_length; i++) {
			a[i] = (int)received_data[i];
		}

        // BU YERDA YURITILADI: Agar mode konfiguratsiya (0x00) bo'lsa, parametrlarni saqlash
        if (a[0] == 0) {
            if (a[1] == 1) { // Motor 1 konfiguratsiyasi
                for(int i = 0; i < data_length && i < 7; i++) {
                    motor1_config[i] = a[i];
                }
                // Motor 1 uchun jami pulslarni hisoblash va saqlash
                int total_degree = motor1_config[5] * 100 + motor1_config[6];
                if (MS_4 != 0) {
                    motor1_total_pulses = (int)(((float)total_degree * (float)MS_4 / 360.0f));
                    motor1_total_pulses *= 2;
                } else {
                    motor1_total_pulses = 0; // MS_4 nol bo'lsa, pulslar ham nol
                }
            } else if (a[1] == 0) { // Motor 2 konfiguratsiyasi
                for(int i = 0; i < data_length && i < 7; i++) {
                    motor2_config[i] = a[i];
                }
                // Motor 2 uchun jami pulslarni hisoblash va saqlash
                int total_degree = motor2_config[5] * 100 + motor2_config[6];
                if (MS_4 != 0) {
                    motor2_total_pulses = (int)(((float)total_degree * (float)MS_4 / 360.0f));
                    motor2_total_pulses *= 2;
                } else {
                    motor2_total_pulses = 0; // MS_4 nol bo'lsa, pulslar ham nol
                }
            }
        }

		receive_flag = 1; // Ma'lumotlar tayyor flagini o'rnatish
		receive_step = 0; // Keyingi qabul qilish uchun qadamni nolga qaytarish
        HAL_I2C_EnableListen_IT(hi2c); // Ma'lumotlar qabul qilingandan so'ng yana tinglashni yoqish
	}
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
  * where the assert_param error has occurred.
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
