#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"
#include "motor.h"

#define BTN_HODING_MAX	3000
#define LONG_KEY_TIME	BTN_HODING_MAX * 0.7
#define Short_KEY_TIME	BTN_HODING_MAX * 0.3

#define DUTY_DEFAULT 1;

typedef enum{
	MOTOR_STOP = -1,
	MOTOR_CW,
	MOTOR_CCW
} motor_state_t;

typedef enum{
	Key_Wait,
	Key_Hold,
	Key_Short,
	Key_Long,
	Key_Pause
} key_state_t;

// Uart.c

extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);
extern void Uart2_RX_Interrupt_Enable(int en);


// SysTick.c

extern void SysTick_Run(unsigned int msec);
extern int SysTick_Check_Timeout(void);
extern unsigned int SysTick_Get_Time(void);
extern unsigned int SysTick_Get_Load_Time(void);
extern void SysTick_Stop(void);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);

// Clock.c

extern void Clock_Init(void);

// Key.c

extern void Key_Poll_Init(void);
extern int Key_Get_Pressed(void);
extern int Key_Get_Released(void);
extern void Key_ISR_Enable(int en);


// Timer.c

extern void TIM5_Out_PWM_Generation(unsigned short freq, uint8_t duty, int a0, int a1);
extern void TIMER_Init(void);
extern void TIM2_Interrupt_Enable(int time);
extern void TIM4_Delay_Interrupt_Enable(int time);


// motor.c

extern void MOTOR_Init(void);
extern void motor_cw(uint8_t duty);
extern void motor_ccw(uint8_t duty);
extern void motor_stop(uint8_t duty);