#include "device_driver.h"
#include "timer.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}
// motor
extern volatile int pre_motor_state;
// uart
extern volatile uint8_t Uart_Data_In;
extern volatile uint8_t Uart_Data;
extern volatile uint8_t duty;

// timer
extern volatile uint8_t TIM4_Expired;
extern volatile uint8_t TIM2_Expired;

extern volatile motor_state_t cur_motor_state;
extern volatile key_state_t key_state;

extern double btn_holding_time;


void EXTI15_10_IRQHandler(void)
{
	
	if(key_state == Key_Wait)
	{
		printf("Key Wait!! -> Key_Hold\n");
		key_state = Key_Hold;
		// Key가 눌리면 UART로 데이터를 받으면 안되기 때문에 Disabled 시켜주어야함.
		printf("TIM2 START!!\n");
		TIM2_Interrupt_Enable(3000);
	}
	// Key_Hold일 때 Interrupt가 들어왔다 = 버튼을 떼서 EXTI13 rising_edge
	else if(key_state == Key_Hold)
	{
		printf("Key_Hold\n");

		// TIM2가 expired 되기 전에 버튼을 뗐다면 수동적으로 타이머를 종료해야한다. 
		if(Macro_Check_Bit_Set(TIM2 -> CR1, 0))
		{
			Macro_Clear_Bit(TIM2->SR, 0);
			NVIC_ClearPendingIRQ(28);
			NVIC_DisableIRQ(28);
			Macro_Clear_Bit(TIM2 -> CR1, 0);
			Macro_Clear_Bit(TIM2->DIER, 0);
		}
		if(!btn_holding_time)
		{
			btn_holding_time = ((TIM2 -> ARR) - (TIM2 -> CNT)) / TIM2_1ms_Pls;
		}
		printf("btn_holding_time: %lf\n", btn_holding_time);
		if(btn_holding_time >= LONG_KEY_TIME )
		{
			key_state = Key_Long;
			TIM2_Expired = 1;
		}
		else if(btn_holding_time <= Short_KEY_TIME)
		{
			key_state = Key_Short;
		}
		else
		{
			key_state = Key_Wait;
		}
	}
	
	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);
}

void USART2_IRQHandler(void)
{
	printf("UART RX_INT!!");
    if(Uart_Data_In == 0)
    {
		printf("Uart_Data_In = 0!!");
		Uart_Data = (unsigned char)USART2->DR;
        Uart_Data_In = 1;

        if(Uart_Data == 'S' || Uart_Data == 's')
        {
			cur_motor_state = MOTOR_STOP;
			Uart_Data_In = 0;
        }
		else if(Uart_Data == 'F' || Uart_Data == 'f')
		{
			if(cur_motor_state == MOTOR_CCW)
			{
				key_state = Key_Short;
			}
			else
			{
				cur_motor_state = MOTOR_CW;
				Uart_Data_In = 0;
			}
		}
		else if(Uart_Data == 'R' || Uart_Data == 'r')
		{
			if(cur_motor_state == MOTOR_CW)
			{
				key_state = Key_Short;
			}
			else
			{
				cur_motor_state = MOTOR_CCW;
				Uart_Data_In = 0;
			}
		}
		else if((Uart_Data >= '0' && Uart_Data <= '9'))
		{
			if(Uart_Data == '0')
			{
				cur_motor_state = MOTOR_STOP;
				Uart_Data_In = 0;
			}
			else
			{
				duty = Uart_Data - '0';
				pre_motor_state = MOTOR_STOP;
				Uart_Data_In = 0;
			}
		}
		else
		{
			printf("Invalid Input. Type Again!!!\n");
			(void) USART2 -> DR;
			Uart_Data_In = 0;
		}
    }
	else // Uart_Data_In == 0
	{
		(void)USART2->DR;
	}

    NVIC_ClearPendingIRQ(38);
}

void TIM4_IRQHandler(void)
{
    Macro_Clear_Bit(TIM4->SR, 0);
    NVIC_ClearPendingIRQ(30);

    TIM4_Expired = 1;
    //한번부르고 인터럽트 disable
    NVIC_DisableIRQ(30);
    Macro_Clear_Bit(TIM4->CR1, 0);
    Macro_Clear_Bit(TIM4->DIER, 0);
}

void TIM2_IRQHandler(void)
{
    Macro_Clear_Bit(TIM2->SR, 0);
    NVIC_ClearPendingIRQ(28);
	printf("Timer2_Expired\n");
    // TIM2_Expired = 1;
	key_state = Key_Long;
    //한번부르고 인터럽트 disable
    NVIC_DisableIRQ(28);
    Macro_Clear_Bit(TIM2->CR1, 0);
    Macro_Clear_Bit(TIM2->DIER, 0);
}