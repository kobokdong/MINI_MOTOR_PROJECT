#include "controller.h"

MF_Direction MF_Controller_Output(const MF_ControllerState *motor)
{
    if (motor->state == MF_STATE_CW) return MF_DIR_CW;
    if (motor->state == MF_STATE_CCW) return MF_DIR_CCW;
    return MF_DIR_NONE;
}

void MF_Controller_Init(MF_ControllerState *motor)
{
    motor->state = MF_STATE_STOPPED;
    motor->target = MF_DIR_NONE;
    motor->last_direction = MF_DIR_NONE;
    motor->stopped_at = 0u;
    motor->reverse_delay_ms = MF_DEFAULT_REVERSE_MS;
    motor->level = MF_DEFAULT_LEVEL;
}

void MF_Controller_Stop(MF_ControllerState *motor, uint32_t now)
{
    MF_Direction active = MF_Controller_Output(motor);
    if (active != MF_DIR_NONE) {
        motor->last_direction = active;
        motor->stopped_at = now;
    }
    motor->target = MF_DIR_NONE;
    motor->state = MF_STATE_STOPPED;
}

static void RequestDirection(MF_ControllerState *motor,
                             MF_Direction direction, uint32_t now)
{
    MF_Direction active = MF_Controller_Output(motor);
    if (active != MF_DIR_NONE && active != direction) {
        MF_Controller_Stop(motor, now);
    }
    motor->target = direction;

    /* A STOP followed by an opposite command must not bypass the dead time. */
    if (motor->last_direction != MF_DIR_NONE &&
        motor->last_direction != direction &&
        (uint32_t)(now - motor->stopped_at) < motor->reverse_delay_ms &&
        MF_Controller_Output(motor) == MF_DIR_NONE) {
        motor->state = MF_STATE_REVERSING;
    } else {
        motor->state = direction == MF_DIR_CW ? MF_STATE_CW : MF_STATE_CCW;
    }
}

void MF_Controller_Command(MF_ControllerState *motor, unsigned char command, uint32_t now)
{
    switch (command) {
    case 'S': case 's': case '0':
        MF_Controller_Stop(motor, now);
        break;
    case 'F': case 'f':
        RequestDirection(motor, MF_DIR_CW, now);
        break;
    case 'R': case 'r':
        RequestDirection(motor, MF_DIR_CCW, now);
        break;
    default:
        if (command >= '1' && command <= '9') {
            motor->level = (uint8_t)(command - '0');
        }
        break;
    }
}

void MF_Controller_ShortPress(MF_ControllerState *motor, uint32_t now)
{
    if (motor->state == MF_STATE_STOPPED) RequestDirection(motor, MF_DIR_CW, now);
    else if (motor->state == MF_STATE_CW) RequestDirection(motor, MF_DIR_CCW, now);
    else if (motor->state == MF_STATE_CCW) RequestDirection(motor, MF_DIR_CW, now);
    /* Ignore another short press while waiting for a reversal. */
}

void MF_Controller_Update(MF_ControllerState *motor, uint32_t now)
{
    if (motor->state == MF_STATE_REVERSING &&
        motor->target != MF_DIR_NONE &&
        (uint32_t)(now - motor->stopped_at) >= motor->reverse_delay_ms) {
        motor->state = motor->target == MF_DIR_CW ? MF_STATE_CW : MF_STATE_CCW;
    }
}

uint8_t MF_Controller_DutyPercent(const MF_ControllerState *motor)
{
    return (uint8_t)(45u + 5u * motor->level);
}
