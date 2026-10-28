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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "secure_nsc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Non-secure Vector table to jump to (internal Flash Bank2 here)             */
/* Caution: address must correspond to non-secure internal Flash where is     */
/*          mapped in the non-secure vector table                             */
#define VTOR_TABLE_NS_START_ADDR  0x08100000UL

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void NonSecure_Init(void);
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GTZC_S_Init(void);
static void MX_RTC_Init(void);
static void MX_TAMP_RTC_Init(void);
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
  /* SAU/IDAU, FPU and interrupts secure/non-secure allocation setup done */
  /* in SystemInit() based on partition_stm32u585xx.h file's definitions. */

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* Aktifkan Instruction Cache (ICACHE) penuh tanpa wait-state penalty (Desain Sistem Hal. 100) */
  SET_BIT(ICACHE->CR, ICACHE_CR_EN);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
  /* GTZC initialisation */
  MX_GTZC_S_Init();

  /* USER CODE BEGIN SysInit */
  /* Buka akses domain backup VSW (RTC, TAMP, Backup Registers) & aktifkan monitor VBAT */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  SET_BIT(PWR->BDCR1, PWR_BDCR1_MONEN); /* Backup domain voltage monitor enable (Hal. 95) */
  SET_BIT(PWR->BDCR2, PWR_BDCR2_VBE);   /* Supercap / Backup battery Trickle Charger ON (Hal. 99) */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TAMP_RTC_Init();
  /* USER CODE BEGIN 2 */
  /* Konfigurasi Auto-Erase Backup Registers saat Case Open (PC13 TAMP_IN1) terpicu (Desain Sistem Hal. 98) */
  SET_BIT(TAMP->ERCFGR, TAMP_ERCFGR0);
  /* USER CODE END 2 */

  /*************** Setup and jump to non-secure *******************************/

  NonSecure_Init();

  /* Non-secure software does not return, this code is not executed */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief  Non-secure call function
  *         This function is responsible for Non-secure initialization and switch
  *         to non-secure state
  * @retval None
  */
static void NonSecure_Init(void)
{
  funcptr_NS NonSecure_ResetHandler;

  SCB_NS->VTOR = VTOR_TABLE_NS_START_ADDR;

  /* Set non-secure main stack (MSP_NS) */
  __TZ_set_MSP_NS((*(uint32_t *)VTOR_TABLE_NS_START_ADDR));

  /* Get non-secure reset handler */
  NonSecure_ResetHandler = (funcptr_NS)(*((uint32_t *)((VTOR_TABLE_NS_START_ADDR) + 4U)));

  /* Start non-secure state software application */
  NonSecure_ResetHandler();
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GTZC_S Initialization Function
  * @param None
  * @retval None
  */
static void MX_GTZC_S_Init(void)
{

  /* USER CODE BEGIN GTZC_S_Init 0 */

  /* USER CODE END GTZC_S_Init 0 */

  MPCBB_ConfigTypeDef MPCBB_Area_Desc = {0};

  /* USER CODE BEGIN GTZC_S_Init 1 */

  /* USER CODE END GTZC_S_Init 1 */
  MPCBB_Area_Desc.SecureRWIllegalMode = GTZC_MPCBB_SRWILADIS_ENABLE;
  MPCBB_Area_Desc.InvertSecureState = GTZC_MPCBB_INVSECSTATE_NOT_INVERTED;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[0] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[1] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[2] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[3] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[4] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[5] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[6] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[7] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[8] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[9] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[10] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[11] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[12] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[13] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[14] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[15] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[16] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[17] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[18] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[19] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[20] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[21] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[22] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[23] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[24] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[25] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[26] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[27] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[28] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[29] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[30] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_SecConfig_array[31] =   0x00000000;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[0] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[1] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[2] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[3] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[4] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[5] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[6] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[7] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[8] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[9] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[10] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[11] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[12] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[13] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[14] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[15] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[16] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[17] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[18] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[19] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[20] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[21] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[22] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[23] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[24] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[25] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[26] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[27] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[28] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[29] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[30] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_PrivConfig_array[31] =   0xFFFFFFFF;
  MPCBB_Area_Desc.AttributeConfig.MPCBB_LockConfig_array[0] =   0x00000000;
  if (HAL_GTZC_MPCBB_ConfigMem(SRAM3_BASE, &MPCBB_Area_Desc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN GTZC_S_Init 2 */

  /* USER CODE END GTZC_S_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_PrivilegeStateTypeDef privilegeState = {0};
  RTC_SecureStateTypeDef secureState = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_POS1;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  privilegeState.rtcPrivilegeFull = RTC_PRIVILEGE_FULL_NO;
  privilegeState.backupRegisterPrivZone = RTC_PRIVILEGE_BKUP_ZONE_NONE;
  privilegeState.backupRegisterStartZone2 = RTC_BKP_DR0;
  privilegeState.backupRegisterStartZone3 = RTC_BKP_DR0;
  if (HAL_RTCEx_PrivilegeModeSet(&hrtc, &privilegeState) != HAL_OK)
  {
    Error_Handler();
  }
  secureState.rtcSecureFull = RTC_SECURE_FULL_YES;
  secureState.backupRegisterStartZone2 = RTC_BKP_DR0;
  secureState.backupRegisterStartZone3 = RTC_BKP_DR0;
  if (HAL_RTCEx_SecureModeSet(&hrtc, &secureState) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable Calibration
  */
  if (HAL_RTCEx_SetCalibrationOutPut(&hrtc, RTC_CALIBOUTPUT_512HZ) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  /* Inisialisasi awal tanggal/jam jika belum terkonfigurasi (tahun < 26) */
  RTC_DateTypeDef sDateCheck = {0};
  RTC_TimeTypeDef sTimeCheck = {0};
  (void)HAL_RTC_GetTime(&hrtc, &sTimeCheck, RTC_FORMAT_BIN);
  if (HAL_RTC_GetDate(&hrtc, &sDateCheck, RTC_FORMAT_BIN) == HAL_OK)
  {
    if (sDateCheck.Year < 26U)
    {
      /* Set baseline tanggal default ke 28 Oktober 2026 10:00:00 (1793181600) */
      SECURE_SetRTCDateTime(1793181600UL);
    }
  }
  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TAMP Initialization Function
  * @param None
  * @retval None
  */
static void MX_TAMP_RTC_Init(void)
{

  /* USER CODE BEGIN TAMP_Init 0 */

  /* USER CODE END TAMP_Init 0 */

  RTC_SecureStateTypeDef secureState = {0};
  RTC_PrivilegeStateTypeDef privilegeState = {0};

  /* USER CODE BEGIN TAMP_Init 1 */

  /* USER CODE END TAMP_Init 1 */

  /** Enable the RTC Active Tamper
  */
  secureState.tampSecureFull = TAMP_SECURE_FULL_NO;
  secureState.MonotonicCounterSecure = TAMP_MONOTONIC_CNT_SECURE_NO;
  if (HAL_RTCEx_SecureModeSet(&hrtc, &secureState) != HAL_OK)
  {
    Error_Handler();
  }
  privilegeState.tampPrivilegeFull = TAMP_PRIVILEGE_FULL_YES;
  privilegeState.MonotonicCounterPrivilege = TAMP_MONOTONIC_CNT_PRIVILEGE_NO;
  if (HAL_RTCEx_PrivilegeModeSet(&hrtc, &privilegeState) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TAMP_Init 2 */
  /* Aktifkan pendeteksi hardware tamper pada domain VSW:
   * - TAMP1E (PC13): Pembukaan Tutup Meter (Case Open)
   * - TAMP3E (PE6) : Pembukaan Tutup Terminal (Terminal Cover Open)
   * - TAMP7E (PE4) : Induksi Medan Magnet Eksternal <= 500 mT */
  SET_BIT(TAMP->CR1, TAMP_CR1_TAMP1E | TAMP_CR1_TAMP3E | TAMP_CR1_TAMP7E);
  /* USER CODE END TAMP_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOC, LCD_I2C3_SCL_Pin|LCD_I2C3_SDA_Pin|VCAP_SENSE_Pin|AC_FAIL_DET_Pin
                          |OPTICAL_TX_Pin|OPTICAL_RX_Pin|RELAY_STATUS_Pin|RELAY_TRIP_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOA, BTN_SCROLL_UP_Pin|MODEM_TX_Pin|MODEM_RX_Pin|ADE_NSS_Pin
                          |ADE_SCK_Pin|ADE_MISO_Pin|ADE_MOSI_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOB, ADE_IRQ1_Pin|FLASH_SPI2_CS_Pin|FLASH_SPI2_SCK_Pin|FLASH_SPI2_MISO_Pin
                          |FLASH_SPI2_MOSI_Pin|ADE_IRQ0_Pin|BTN_SCROLL_DN_Pin|LED_ALM_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOE, ADE_RESET_Pin|ADE_CF_ZX_Pin|ADE_PM0_Pin|ADE_PM1_Pin
                          |ADE_EVENT_Pin|ADE_CF1_Pin|ADE_CF2_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOD, AUX_CTRL_OUT1_Pin|AUX_CTRL_IN1_Pin|VBAT_SENSE_Pin|EXT_WDI_PULSE_Pin
                          |AUX_CTRL_IN2_Pin|LCD_PWR_EN_Pin|LCD_BL_CTRL_Pin|RS232_PWR_EN_Pin
                          |MODEM_12V_EN_Pin, GPIO_PIN_NSEC);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Setel pin PC15 (UNUSED_PC15) sebagai Analog Mode untuk meminimalkan arus bocor (PIN.docx Baris 17-20) */
  SET_BIT(GPIOC->MODER, GPIO_MODER_MODE15);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param None
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
