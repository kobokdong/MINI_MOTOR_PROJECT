#ifndef BOARD_H
#define BOARD_H
#include "motor_framework.h"
void Board_Init(void);
MF_Port Board_MotorPort(void);
void Board_StartReception(MF_Instance *instance);
#endif
