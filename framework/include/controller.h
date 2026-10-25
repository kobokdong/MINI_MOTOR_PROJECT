#ifndef MF_CONTROLLER_H
#define MF_CONTROLLER_H
#include <stdint.h>

#define MF_DEFAULT_REVERSE_MS 1000u
#define MF_DEFAULT_LEVEL 1u

typedef enum {
    MF_STATE_STOPPED,
    MF_STATE_CW,
    MF_STATE_CCW,
    MF_STATE_REVERSING
} MF_MotorState;

typedef enum { MF_DIR_NONE, MF_DIR_CW, MF_DIR_CCW } MF_Direction;

/* Owned exclusively by the main loop; interrupts never access this structure. */
typedef struct {
    MF_MotorState state;
    MF_Direction target;
    MF_Direction last_direction;
    uint32_t stopped_at;
    uint32_t reverse_delay_ms;
    uint8_t level;
} MF_ControllerState;

void MF_Controller_Init(MF_ControllerState *motor);
void MF_Controller_Stop(MF_ControllerState *motor, uint32_t now);
void MF_Controller_Command(MF_ControllerState *motor, unsigned char command, uint32_t now);
void MF_Controller_ShortPress(MF_ControllerState *motor, uint32_t now);
void MF_Controller_Update(MF_ControllerState *motor, uint32_t now);
MF_Direction MF_Controller_Output(const MF_ControllerState *motor);
uint8_t MF_Controller_DutyPercent(const MF_ControllerState *motor);
#endif
