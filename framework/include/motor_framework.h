#ifndef MOTOR_FRAMEWORK_H
#define MOTOR_FRAMEWORK_H
#include <stdint.h>
#include "controller.h"
#include "button.h"
#include "input.h"

#define MF_VERSION "1.0.0"

typedef struct {
    uint32_t reverse_delay_ms;
    uint32_t debounce_ms;
    uint32_t short_press_ms;
    uint32_t long_release_ms;
    uint32_t hold_stop_ms;
    uint8_t default_level;
} MF_Config;

typedef struct {
    void *user;
    uint32_t (*enter_critical)(void *user);
    void (*exit_critical)(void *user, uint32_t saved);
    uint32_t (*now_ms)(void *user);
    uint8_t (*button_pressed)(void *user);
    /* Must be nonblocking, bounded, and must not reenter MF APIs. */
    void (*apply_output)(void *user, MF_Direction direction, uint8_t percent);
} MF_Port;

typedef enum {
    MF_COMMAND_CW, MF_COMMAND_CCW, MF_COMMAND_STOP, MF_COMMAND_LEVEL
} MF_CommandType;

typedef struct {
    MF_MotorState state;
    MF_Direction target;
    MF_Direction output;
    uint8_t level;
    uint8_t duty_percent;
    uint32_t rx_errors;
    uint32_t rx_overflows;
    uint8_t stop_pending;
} MF_Status;

/* Statically allocated context; fields are implementation state, not public setters.
 * Each instance needs its own context and a distinct hardware output binding. */
typedef struct {
    MF_Config config;
    MF_Port port;
    MF_ControllerState controller;
    MF_ButtonState button;
    MF_InputBuffer input;
    MF_Direction applied_direction;
    uint8_t applied_percent;
    uint8_t initialized;
} MF_Instance;

MF_Config MF_DefaultConfig(void);
/* Call once before enabling producer interrupts. Returns 0 on invalid config/port.
 * Calling Init on a running instance is unsupported; stop and detach interrupts first. */
int MF_Init(MF_Instance *instance, const MF_Config *config, const MF_Port *port);
/* Call frequently from one main loop; never from an ISR. */
void MF_Process(MF_Instance *instance);
/* Main-context APIs; submission is asynchronous and applied by the next Process. */
int MF_Command(MF_Instance *instance, MF_CommandType command, uint8_t level);
void MF_Stop(MF_Instance *instance);
int MF_GetStatus(MF_Instance *instance, MF_Status *status);
/* One serialized ISR producer per instance; main APIs use port critical hooks. */
void MF_OnByteISR(MF_Instance *instance, unsigned char byte);
void MF_OnRxErrorISR(MF_Instance *instance);
#endif
