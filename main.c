#include "device_driver.h"
#include <stdio.h>

// #define MOTOR_STOP_TIME	3000
// #define MOTOR_STOP_TIME_REF	1000

volatile uint8_t TIM2_Expired = 0;
volatile uint8_t TIM4_Expired = 0;
volatile uint8_t duty = DUTY_DEFAULT;
volatile uint8_t Uart_Data_In = 0;
volatile unsigned char Uart_Data = 0;
volatile motor_state_t cur_motor_state;
volatile key_state_t key_state = Key_Wait;
volatile int pre_motor_state = -2;
volatile double btn_holding_time = 0;

static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Key_Poll_Init();
	MOTOR_Init();
	TIMER_Init();
}

void Main(void)
{
	int motor_tmp = MOTOR_STOP;
	cur_motor_state = MOTOR_STOP;

    Sys_Init(115200);
    printf("\nstart\n");

	Uart2_RX_Interrupt_Enable(1);
	Key_ISR_Enable(1);

	for(;;)
	{
		if(pre_motor_state != cur_motor_state)	
		{
			switch(cur_motor_state)
			{
				case -1:
					printf("Motor Stop\n");
					motor_stop(duty);
					break;
				case 0:
					printf("cw\n");
					motor_cw(duty);
					break;
				case 1:
					printf("ccw\n");
					motor_ccw(duty);
					break;
					}
			pre_motor_state = cur_motor_state;
		}

		if(TIM2_Expired)
		{
			cur_motor_state = MOTOR_STOP;
			TIM2_Expired = 0;
			btn_holding_time = BTN_HODING_MAX;
		}

		if(key_state == Key_Short)
		{
			printf("Key_Short\n");
			if(cur_motor_state == MOTOR_STOP)
			{
				cur_motor_state = MOTOR_CW;
				key_state = Key_Wait;
				Uart2_RX_Interrupt_Enable(1);
			}
			
			else // 모터가 동작중일 때
			{
				motor_tmp = !cur_motor_state;
				cur_motor_state = MOTOR_STOP;
				printf("TIM4 Start\n");
				TIM4_Delay_Interrupt_Enable(1000);
				key_state = Key_Pause;
			}
		}
		else if(key_state == Key_Long)
		{
			printf("key_state: %d\n", key_state);
			cur_motor_state = MOTOR_STOP;
			btn_holding_time = 0;
			key_state = Key_Wait;
			Uart2_RX_Interrupt_Enable(1);
		}

		if(TIM4_Expired)
		{
			printf("MOTOR toggle\n");
			cur_motor_state = motor_tmp;
			if(Key_Get_Released() || Uart_Data_In)
			{
				key_state = Key_Wait;
				TIM4_Expired = 0;
				(void) USART2 ->DR;
				if(!Uart_Data_In)
				{
					Uart2_RX_Interrupt_Enable(1);
				}
				else
				{
					Key_ISR_Enable(1);
				}
				Uart_Data_In = 0;
			}
		}
	}
}