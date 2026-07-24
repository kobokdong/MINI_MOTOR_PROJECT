#include "device_driver.h"
#include <stdio.h>
#include "timer.h"

extern uint8_t TIM4_Expired;

void TIMER_Init(void)
{
	Macro_Write_Block(RCC->APB1ENR, 0xf, 0xd, 0);
	Macro_Write_Block(GPIOA->AFR[0], 0xff, 0x22, 0); 	// PB0 => AF02
	Macro_Write_Block(TIM5->CCMR1,0xffff, 0x6060, 0);
	TIM5->CCER = (0x1<<4)|(0x1<<0);
}

void TIM2_Interrupt_Enable(int time)
{
	TIM2->CR1 = (0x1 << 4)|(0x1 << 3)|(0x0 << 0);
	TIM2->PSC = (int)(TIMXCLK/TIM2_FREQ + 0.5)-1;
	TIM2->ARR = TIM2_1ms_Pls * time;
	Macro_Set_Bit(TIM2->EGR,0);

	Macro_Clear_Bit(TIM2->SR, 0);
	NVIC_ClearPendingIRQ(28);
	Macro_Set_Bit(TIM2->DIER, 0);
	NVIC_EnableIRQ(28);

	Macro_Set_Bit(TIM2->CR1, 0);
}

void TIM4_Delay_Interrupt_Enable(int time)
{
	TIM4_Expired = 0;
	TIM4->CR1 = (0x1 << 4)|(0x1 << 3)|(0x0 << 0);
	TIM4->PSC = (int)(TIMXCLK/TIM4_FREQ + 0.5)-1;
	TIM4->ARR = TIM4_1ms_Pls * time;
	Macro_Set_Bit(TIM4->EGR,0);

	Macro_Clear_Bit(TIM4->SR, 0);
	NVIC_ClearPendingIRQ(30);
	Macro_Set_Bit(TIM4->DIER, 0);
	NVIC_EnableIRQ(30);

	Macro_Set_Bit(TIM4->CR1, 0);
}

void TIM5_Out_PWM_Generation(unsigned short freq, uint8_t duty, int a0, int a1)
{
	//a0, a1= 1이면 PWM 동작(MODER에 0x2), 0이면 GPIOA 출력(MODER에 0x1)
	Macro_Write_Block(GPIOA->MODER, 0xf, ((a1 + 1) << 2 | (a0 + 1)), 0);

	TIM5 -> CR1 |= (0x0 << 7) | (0x1 << 4) | (0x0 << 3) | (0x0 << 0);
	// Timer 주파수가 TIM3_FREQ가 되도록 PSC 설정
	TIM5 -> PSC = (int)((double)TIMXCLK / TIM5_FREQ + 0.5) - 1;
	// 요청한 주파수가 되도록 ARR 설정
	TIM5 -> ARR = (int)((double)(TIM5_FREQ / freq) + 0.5);
	// Duty Rate 50%가 되도록 CCR3 설정
	TIM5 -> CCR1 = (int)((TIM5 -> ARR) * ((45 + 5 * duty) / 100.));
	TIM5 -> CCR2 = (int)((TIM5 -> ARR) * ((45 + 5 * duty) / 100.));
	// Manual Update(UG 발생)
	TIM5 -> EGR |= 0x1 << 0;
	// Down Counter, Repeat Mode, Timer Start
	TIM5 -> CR1 |= (0x1 << 0);

}