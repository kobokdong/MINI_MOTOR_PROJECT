#ifndef DEVICE_DRIVER_H
#define DEVICE_DRIVER_H
#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "controller.h"

void Clock_Init(void);
void Uart2_Init(unsigned int baud);
void Uart2_RX_Interrupt_Enable(void);
void MotorDriver_Init(void);
void MotorDriver_Apply(MF_Direction direction, uint8_t percent);
void Key_Init(void);
uint8_t Key_IsPressed(void);
void Timebase_Init(void);
uint32_t Timebase_Now(void);

#endif
