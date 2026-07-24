#include "device_driver.h"
#include <stdio.h>

extern volatile uint8_t duty;

void MOTOR_Init(void)
{
    Macro_Write_Block(GPIOA -> MODER, 0xf, 0x5, 0);
	GPIOA->OTYPER &= ~(0x3 <<0) ;
	Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);

}

void motor_stop(uint8_t duty)
{
    TIM5_Out_PWM_Generation(10000, duty, 0, 0);
}

void motor_ccw(uint8_t duty)
{
	//Macro_Write_Block(GPIOA->ODR, 0x3, 0x1, 0);
    TIM5_Out_PWM_Generation(10000, duty, 1, 0);
}

void motor_cw(uint8_t duty)
{
	//Macro_Write_Block(GPIOA->ODR, 0x3, 0x2, 0);
    TIM5_Out_PWM_Generation(10000, duty, 0, 1);
}

