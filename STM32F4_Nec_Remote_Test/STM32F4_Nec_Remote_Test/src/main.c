#include "stm32f4xx_conf.h"
#include <stdio.h>
#include "ir_decode.h"
#include "stm32f4_discovery.h"

static volatile uint32_t ticksms = 0;
static volatile uint32_t ticksus = 0;

void Int2Str(char *str, u32 intnum) {
	u32 Div = 1000000000;
	int i, j = 0, Status = 0;
	for (i = 0; i < 10; i++) {
		str[j++] = (intnum / Div) + 48;
		intnum = intnum % Div;
		Div /= 10;
		if ((str[j - 1] == '0') & (Status == 0)) {
			str[j] = '\0';
			j = 0;
		} else {
			str[j] = '\0';
			Status++;
		}
	}
}

void delayMS(uint32_t ms) {
	uint32_t t = ticksms + ms;
	while (t > ticksms)
		;
}

void delayUS(uint32_t us) {
	uint32_t t = ticksus + us;
	while (t > ticksus)
		;
}

void USART_WriteString(volatile char *s) {

	while (*s) {
		// wait until data register is empty
		while (!(USART2->SR & 0x00000040))
			;
		USART_SendData(USART2, *s);
		*s++;
	}
}

void init_usart(uint32_t baudrate) {

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* enable peripheral clock for USART2 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* GPIOA Configuration:  USART2 TX on PA2 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Connect USART2 pins to AF2 */
	// TX = PA2
	// RX = PA3
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl
			= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	USART_Cmd(USART2, ENABLE); // enable USART2

}

void init_GPIO() {

	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_7 | GPIO_Pin_13
			| GPIO_Pin_14 | GPIO_Pin_15; // we want to configure all LED GPIO pins
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT; // we want the pins to be an output
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; // this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL; // this sets the pullup / pulldown resistors to be inactive
	GPIO_Init(GPIOD, &GPIO_InitStruct); // this finally passes all the values to the GPIO_Init function which takes care of setting the corresponding bits.




}





void TIM2_IRQHandler() {
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		NEC_TimerRanOut();
		//GPIO_ToggleBits(GPIOD, GPIO_Pin_7);

	}
}

void EXTI0_IRQHandler() {
	if (EXTI_GetITStatus(IR_EXTI_LINE) != RESET) {
		NEC_HandleEXTI();
	}
	EXTI_ClearITPendingBit(IR_EXTI_LINE);
}

void NEC_ReceiveInterrupt(NEC_FRAME f) {
	GPIO_ToggleBits(GPIOD, GPIO_Pin_7);
	char buf[12];
	USART_WriteString("NEC Frame was received : \r\nAddress : ");
	Int2Str(buf, f.Address);
	USART_WriteString(buf);
	USART_WriteString("\r\nCommand : ");
	Int2Str(buf, f.Command);
	USART_WriteString(buf);
	USART_WriteString("\r\n");


	switch (f.Command) {
	case 12:
		GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
		break;
	case 24:
		GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
		break;
	case 94:
		GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
		break;
	case 8:
		GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
		break;
	}

}

int main(void) {

	SystemInit();
	init_usart(115200);
	init_GPIO();
	//init_timer();
	NEC_Init();
	//STM_EVAL_LEDInit(LED4);
	//STM_EVAL_LEDInit(LED5);

	//STM_EVAL_LEDOn(LED4);

	USART_WriteString("NEC Remote Control-Welcome!\r\n");

	while (1) {

	}

	return 0;
}
