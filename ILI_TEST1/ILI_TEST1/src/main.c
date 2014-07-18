/**
  ******************************************************************************
  * @file    main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    29-July-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi_conf.h"
#include "bitmap.h"

/** @addtogroup STM32L100C-Discovery_Demo
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
uint8_t BlinkSpeed = 0;

#include "Lcd_Driver.h"
#include "GUI.h"
#include "delay.h"
#include "clock.h"
#include "stdbool.h"

#include "TFT_TEXT/mk_text.h"
#include "TFT_FONT/mk_fonts.h"
//#include "TFT_FONT/LEDBOARD36pt.h"

#include "math.h"


#define SYSCLK_FREQ  HSI_VALUE
#define SYSTICK_FREQ 100 // 100 Hz -> 10 ms

/* Private define ------------------------------------------------------------*/
#define DAC_DHR12R2_Address      0x40007414

/* 8-bit */
#define DAC_DHR8R1_Address       0x40007410

/* channel1 12-bit left aligned data holding register */
#define DAC_DHR12L1_Address       0x4000740C

/* DAC channel1 12-bit right-aligned data holding register */
#define DAC_DHR12R1_Address       0x40007408

GPIO_InitTypeDef GPIO_InitStructure;


//void RCC_Configuration(void);

//void Delayms(__IO uint32_t nCount);

unsigned char Num[10]={0,1,2,3,4,5,6,7,8,9};
volatile bool updated = false;

void GPIO_Config(void)
{

 GPIO_InitTypeDef  GPIO_InitStructure;

 /*************************************/
 /*!< SD_SPI_CS_GPIO, SD_SPI_MOSI_GPIO, SD_SPI_MISO_GPIO, SD_SPI_DETECT_GPIO
       and SD_SPI_SCK_GPIO Periph clock enable */
  RCC_AHBPeriphClockCmd(SD_CS_GPIO_CLK | SD_SPI_MOSI_GPIO_CLK | SD_SPI_MISO_GPIO_CLK |
                        SD_SPI_SCK_GPIO_CLK | SD_DETECT_GPIO_CLK, ENABLE);

  /*!< SD_SPI Periph clock enable */
  RCC_APB2PeriphClockCmd(SD_SPI_CLK, ENABLE);

  /*!< Configure SD_SPI pins: SCK */
  GPIO_InitStructure.GPIO_Pin = SD_SPI_SCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(SD_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

  /*!< Configure SD_SPI pins: MISO */
  GPIO_InitStructure.GPIO_Pin = SD_SPI_MISO_PIN;
  GPIO_Init(SD_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

  /*!< Configure SD_SPI pins: MOSI */
  GPIO_InitStructure.GPIO_Pin = SD_SPI_MOSI_PIN;
  GPIO_Init(SD_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

  /*!< Configure SD_SPI_CS_PIN pin: SD Card CS pin */
  GPIO_InitStructure.GPIO_Pin = SD_CS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_Init(SD_CS_GPIO_PORT, &GPIO_InitStructure);

  /*!< Configure SD_SPI_DETECT_PIN pin: SD Card detect pin */
  GPIO_InitStructure.GPIO_Pin = SD_DETECT_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(SD_DETECT_GPIO_PORT, &GPIO_InitStructure);

  /* Connect PXx to SD_SPI_SCK */
  GPIO_PinAFConfig(SD_SPI_SCK_GPIO_PORT, SD_SPI_SCK_SOURCE, SD_SPI_SCK_AF);

  /* Connect PXx to SD_SPI_MISO */
  GPIO_PinAFConfig(SD_SPI_MISO_GPIO_PORT, SD_SPI_MISO_SOURCE, SD_SPI_MISO_AF);

  /* Connect PXx to SD_SPI_MOSI */
  GPIO_PinAFConfig(SD_SPI_MOSI_GPIO_PORT, SD_SPI_MOSI_SOURCE, SD_SPI_MOSI_AF);
 /**************************************/

}

void SPI_Config(void)
{
  //konfigurowanie interfejsu SPI
  SPI_InitTypeDef   SPI_InitStructure;

  SPI_InitStructure.SPI_Direction =  SPI_Direction_2Lines_FullDuplex;//transmisja z wykorzystaniem jednej linii, transmisja jednokierunkowa
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                     //tryb pracy SPI
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b ;                //8-bit ramka danych
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                        //stan sygnalu taktujacego przy braku transmisji - wysoki
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                      //aktywne zbocze sygnalu taktujacego - 2-gie zbocze
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                         //programowa obsluga linii NSS (CS)
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;//prescaler szybkosci tansmisji  72MHz/4=18MHz
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                //pierwszy bit w danych najbardziej znaczacy
  SPI_InitStructure.SPI_CRCPolynomial = 7;                          //stopien wielomianu do obliczania sumy CRC
  SPI_Init(SPI1, &SPI_InitStructure);                               //inicjalizacja SPI

  SPI_Cmd(SPI1, ENABLE);  	// Wlacz SPI1
}


/* Private function prototypes -----------------------------------------------*/

static FATFS g_sFatFs; //obiekt FATFs
TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;
DAC_InitTypeDef            DAC_InitStructure;
DMA_InitTypeDef            DMA_InitStructure;
const uint16_t Sine12bit[32] = {
                      2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056,
                      3939, 3750, 3495, 3185, 2831, 2447, 2047, 1647, 1263, 909,
                      599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647};
extern uint8_t buffer[2][512];
extern FIL     plik;
extern UINT bytesToRead,bytesRead;
bool canRead;
extern uint8_t i;

const uint8_t Escalator8bit[6] = {0x0, 0x33, 0x66, 0x99, 0xCC, 0xFF};
uint8_t Idx = 0;
__IO uint8_t  SelectedWavesForm = 1;
__IO uint8_t WaveChange = 1;

/* Private function prototypes -----------------------------------------------*/
void DAC_PreConfig(void);

#define TABLE_SIZE 512

#define TWO_PI (3.14159 * 2)

float samples [TABLE_SIZE];

float phaseIncrement = TWO_PI/TABLE_SIZE;

float currentPhase = 0.0;

int main(void)
{
	//u8 i=2;
	uint16_t i1;
	FRESULT fresult;

	UINT bytesToRead,bytesRead;
	//uint8_t buffer[1024];

	NVIC_InitTypeDef nvicStructure;


	 SystemInit();

	 /*
	 for(i1=0;i1<TABLE_SIZE;i1++){
		 buffer[0][i1] = 4*i1;
	 }

	 for(i1=0;i1<TABLE_SIZE;i1++){
	 		 buffer[1][i1] = 2047*sin(currentPhase)+2047;
	 		currentPhase += phaseIncrement;
	 	 }
	 	 */


	 /* Configure LED3 and LED4 on STM32L100C-Discovery */
	 	      STM_EVAL_LEDInit(LED3);
	 	      STM_EVAL_LEDInit(LED4);

	 /* Preconfiguration before using DAC----------------------------------------*/
	DAC_PreConfig();

	/* TIM2 Configuration ------------------------------------------------------*/
	/* TIM2 Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	//TIM_TimeBaseStructure.TIM_Period = 0x20;
	TIM_TimeBaseStructure.TIM_Period = 0x80;
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0A;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

	/* TIM2 TRGO selection */
	TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

	/* Enable the TIM6 Interrupt */
	/*
	 nvicStructure.NVIC_IRQChannel = TIM6_IRQn;
	 nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
	 nvicStructure.NVIC_IRQChannelSubPriority = 0;
	 nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	 NVIC_Init(&nvicStructure);
	 */

	/* TIM2 enable counter */
	TIM_Cmd(TIM6, ENABLE);

	/****************************/
	/* Triangle Wave generator -----------------------------------------*/
	DAC_DeInit();

	/* Noise Wave generator --------------------------------------------*/
	/* DAC channel1 Configuration */

	//DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

	DMA_DeInit(DMA1_Channel2);
	//DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR12R1_Address;
	DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR8R1_Address;
	//DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&Sine12bit;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) &buffer[0][0];
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	//DMA_InitStructure.DMA_BufferSize = 32;
	DMA_InitStructure.DMA_BufferSize = 512;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	//DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	//DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
	//  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(DMA1_Channel2, &DMA_InitStructure);

	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

	/* Enable the DMA1_Channel3 Interrupt */
	nvicStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 1;
	nvicStructure.NVIC_IRQChannelSubPriority = 3;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;

	/* Enable DMA for DAC Channel1 */
	DAC_DMACmd(DAC_Channel_1, ENABLE);

	GPIO_Config();
	SPI_Config();

	fresult = f_mount(&g_sFatFs, "0:0", 1);



	//fresult = f_open(&plik,"z.wav", FA_READ);
	fresult = f_open(&plik, "bj8.wav", FA_READ);

	bytesToRead = f_size(&plik);

	//fresult = f_lseek(&plik, 44 + 22050);
	fresult = f_lseek(&plik, 0);

	fresult = f_read(&plik, &buffer[0][0], 512 , &bytesRead);
	fresult = f_read(&plik, &buffer[1][0], 512 , &bytesRead);

	NVIC_Init(&nvicStructure);

	DMA_Cmd(DMA1_Channel2, ENABLE);

	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	DAC_Cmd(DAC_Channel_1, ENABLE);

	canRead = false;
	i = 0;

	while (1) {
		if (canRead == true) {
			f_read(&plik, &buffer[i ^ 0x01][0], 512 , &bytesRead);

			//tmp = ((*pTmp) + 32768)>>4;

			/*
			 if((i) == 0){
			 f_read(&plik, &buffer[1][0], 512*2, &bytesRead);
			 } else {
			 f_read(&plik, &buffer[0][0], 512*2, &bytesRead);
			 }
			 */

			canRead = false;
			if (bytesRead < 512)
				break;
		}

	}

	fresult = f_close(&plik);








	  	                                  //
	  	                                           //DAC_SetChannel1Data(DAC_Align_12b_R,0x0AAF);
	   /*****************************/
//while(1);


	 	  /* SysTick end of count event each 1ms */
	    //RCC_GetClocksFreq(&RCC_Clocks);
	   // SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

	  //delay_init(72);//

	  delay_init(16);
	  Lcd_Init2();

	  Lcd_Clear2(BLACK);

	  /* RTC configuration */
	    //RTC_Config();
	  RTC_Config32768Internal();

	  	/* Configure RTC alarm A register to generate 8 interrupts per 1 Second */
	    RTC_AlarmConfig();





	    tft_puts(45,20, "12:00", white, black);



	    RCC_ClocksTypeDef RCC_Clocks;



	      /* Initialize User_Button on STM32L100C-Discovery */
	      STM_EVAL_PBInit(BUTTON_MODE, BUTTON_MODE_GPIO);
	      STM_EVAL_PBInit(BUTTON_UP, BUTTON_MODE_GPIO);
	      STM_EVAL_PBInit(BUTTON_DOWN, BUTTON_MODE_GPIO);
	      //STM_EVAL_PBInit(BUTTON_MODE, BUTTON_MODE_EXTI);

	      /* SysTick end of count event each 1ms */
	   //   RCC_GetClocksFreq(&RCC_Clocks);
	   //   SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);


	      /*************************/

	      	  //TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	      	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);


			  nvicStructure.NVIC_IRQChannel = TIM2_IRQn;
			  nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
			  nvicStructure.NVIC_IRQChannelSubPriority = 1;
			  nvicStructure.NVIC_IRQChannelCmd = ENABLE;
			  NVIC_Init(&nvicStructure);

	      	  TIM_TimeBaseInitTypeDef timerInitStructure;
	      	  timerInitStructure.TIM_Prescaler = 100;
	      	  timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	      	  timerInitStructure.TIM_Period = 3332;
	      	  timerInitStructure.TIM_ClockDivision = 0;

	      	  //timerInitStructure.TIM_RepetitionCounter = 0;
	      	  TIM_TimeBaseInit(TIM2, &timerInitStructure);
	      	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	      	  TIM_Cmd(TIM2, ENABLE);



	      	  /*************************/

	      	//GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI1);
	      	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
	      	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
	      	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);



  //LCD_Write_Colon(105,20);
  //LCD_Write_Colon(180,20);
   
  //displayDate();

	      	LCD_BMP("kasia.bmp");


  //setCurrentFont( &DefaultFontInfo);
  //tft_puts(0,130, "12:34", red, white);


  //fillRect(10, 130, 20, 20, RED);

  while(1)
  {  
	  if(updated){
		  displayTime();
		  updated= false;
	  }



  }


}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 1 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}





#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/**
  * @}
  */

static void RTC_Config32768Internal(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  RTC_DateTypeDef  RTC_DateStruct;

//uint8_t yr;

  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	 /* SYSCFG Peripheral clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Allow access to RTC */
  PWR_RTCAccessCmd(ENABLE);

/* LSI used as RTC source clock */
/* The RTC Clock may varies due to LSI frequency dispersion. */
  /* Enable the LSI OSC */
  //RCC_LSICmd(ENABLE);

	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

	RCC_LSEConfig(RCC_LSE_ON);

  /* Wait till LSI is ready */
  //while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }





  /* Select the RTC Clock Source */
  //RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);


  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();

  // LSE 32768
  RTC_InitStructure.RTC_AsynchPrediv = 127;
  RTC_InitStructure.RTC_SynchPrediv  = 255;

	/* Calendar Configuration with LSI supposed at 37KHz */
	//RTC_InitStructure.RTC_AsynchPrediv = 124;
  //RTC_InitStructure.RTC_SynchPrediv  = 295; /* (32KHz / 128) - 1 = 0xFF*/
  RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
  RTC_Init(&RTC_InitStructure);

  /* Set the time to 05h 20mn 00s AM */
  RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
  RTC_TimeStructure.RTC_Hours   = 0x12;
  RTC_TimeStructure.RTC_Minutes = 0x00;
  RTC_TimeStructure.RTC_Seconds = 0x00;

	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);

	RTC_DateStruct.RTC_Year = 14;
	RTC_DateStruct.RTC_Month = 7;
	RTC_DateStruct.RTC_Date = 5;
	RTC_DateStruct.RTC_WeekDay = 6;

	RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct);

	//RTC_GetDate(RTC_Format_BCD, &RTC_DateStruct);

	//yr = RTC_DateStruct.RTC_Year;

	/* EXTI configuration *******************************************************/
  EXTI_ClearITPendingBit(EXTI_Line20);
  EXTI_InitStructure.EXTI_Line = EXTI_Line20;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

// Configuring RTC_WakeUp interrupt
  NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

// RTC Wakeup Configuration
RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);

// RTC Set WakeUp Counter
RTC_SetWakeUpCounter(0);

// Enabling RTC_WakeUp interrupt
RTC_ITConfig(RTC_IT_WUT, ENABLE);

// Disabling Alarm Flags
RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
RTC_AlarmCmd(RTC_Alarm_B, DISABLE);

// RTC Enable the Wakeup Function
RTC_WakeUpCmd(ENABLE);

}





/**
  * @brief  Configures the RTC Alarm.
  * @param  None
  * @retval None
  */
static void RTC_AlarmConfig(void)
{
  EXTI_InitTypeDef EXTI_InitStructure;
  RTC_AlarmTypeDef RTC_AlarmStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* EXTI configuration */
  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable the RTC Alarm Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Set the alarm A Masks */
  RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_All;
  RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
  RTC_AlarmStructure.RTC_AlarmDateWeekDay = RTC_Weekday_Monday;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours = 0x0;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes =0x0;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds =0x0;
  RTC_SetAlarm(RTC_Format_BCD, RTC_Alarm_A, &RTC_AlarmStructure);

  /* Set alarm A sub seconds and enable SubSec Alarm : generate 8 interrupts per Second */
  RTC_AlarmSubSecondConfig(RTC_Alarm_A, 0xFF, RTC_AlarmSubSecondMask_SS14_5);

  /* Enable alarm A interrupt */
  RTC_ITConfig(RTC_IT_ALRA, ENABLE);
}

/**
  * @brief  PrecConfiguration: configure PA4 and PA5 in analog,
  *                           enable DAC clock, enable DMA1 clock
  * @param  None
  * @retval None
  */
void DAC_PreConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* DMA1 clock enable (to be used with DAC) */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  /* DAC Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  /* GPIOA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  /* Configure PA.04 (DAC_OUT1), PA.05 (DAC_OUT2) as analog */
  //GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
