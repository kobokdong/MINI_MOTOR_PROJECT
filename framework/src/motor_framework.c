#include "motor_framework.h"

MF_Config MF_DefaultConfig(void)
{
    MF_Config config = {
        MF_DEFAULT_REVERSE_MS, MF_BUTTON_DEBOUNCE_MS, MF_BUTTON_SHORT_MS,
        MF_BUTTON_LONG_RELEASE_MS, MF_BUTTON_HOLD_STOP_MS, MF_DEFAULT_LEVEL
    };
    return config;
}

static int Valid(const MF_Config *c, const MF_Port *p)
{
    const uint32_t limit = 0x7fffffffu;
    return c && p && p->enter_critical && p->exit_critical && p->now_ms &&
        p->button_pressed && p->apply_output &&
        c->reverse_delay_ms > 0u && c->reverse_delay_ms <= limit &&
        c->debounce_ms > 0u && c->debounce_ms <= c->short_press_ms &&
        c->short_press_ms < c->long_release_ms &&
        c->long_release_ms <= c->hold_stop_ms && c->hold_stop_ms <= limit &&
        c->default_level >= 1u && c->default_level <= 9u;
}

int MF_Init(MF_Instance *instance, const MF_Config *config, const MF_Port *port)
{
    uint32_t saved, now;
    if (!instance) return 0;
    instance->initialized = 0u;
    if (!Valid(config, port)) return 0;
    instance->config = *config;
    instance->port = *port;
    saved = port->enter_critical(port->user);
    now = port->now_ms(port->user);
    MF_Input_Init(&instance->input);
    MF_Controller_Init(&instance->controller);
    instance->controller.reverse_delay_ms = config->reverse_delay_ms;
    instance->controller.level = config->default_level;
    MF_Button_Init(&instance->button, port->button_pressed(port->user) != 0u, now);
    instance->button.debounce_ms = config->debounce_ms;
    instance->button.short_ms = config->short_press_ms;
    instance->button.long_release_ms = config->long_release_ms;
    instance->button.hold_stop_ms = config->hold_stop_ms;
    instance->applied_direction = MF_DIR_NONE;
    instance->applied_percent = MF_Controller_DutyPercent(&instance->controller);
    port->apply_output(port->user, MF_DIR_NONE, instance->applied_percent);
    instance->initialized = 1u;
    port->exit_critical(port->user, saved);
    return 1;
}

void MF_Process(MF_Instance *instance)
{
    uint32_t saved, now;
    MF_ButtonEvent event;
    unsigned char command;
    MF_Direction output;
    uint8_t percent;
    MF_Port *port;
    if (!instance || !instance->initialized) return;
    port = &instance->port;
    saved = port->enter_critical(port->user);
    now = port->now_ms(port->user);
    event = MF_Button_Update(&instance->button, port->button_pressed(port->user) != 0u, now);

    if (MF_Input_TakeStop(&instance->input) || event == MF_BUTTON_LONG) {
        MF_Input_Discard(&instance->input);
        MF_Controller_Stop(&instance->controller, now);
        MF_Button_Cancel(&instance->button);
    } else {
        if (event == MF_BUTTON_SHORT) MF_Controller_ShortPress(&instance->controller, now);
        if (MF_Input_Pop(&instance->input, &command)) {
            MF_Controller_Command(&instance->controller, command, now);
        }
        MF_Controller_Update(&instance->controller, now);
    }

    output = MF_Controller_Output(&instance->controller);
    percent = MF_Controller_DutyPercent(&instance->controller);
    if (output != instance->applied_direction ||
        (output != MF_DIR_NONE && percent != instance->applied_percent)) {
        port->apply_output(port->user, output, percent);
    }
    instance->applied_direction = output;
    instance->applied_percent = percent;
    port->exit_critical(port->user, saved);
}

int MF_Command(MF_Instance *instance, MF_CommandType command, uint8_t level)
{
    unsigned char byte;
    uint32_t saved;
    if (!instance || !instance->initialized) return 0;
    switch (command) {
    case MF_COMMAND_CW: byte = 'F'; break;
    case MF_COMMAND_CCW: byte = 'R'; break;
    case MF_COMMAND_STOP: byte = 'S'; break;
    case MF_COMMAND_LEVEL:
        if (level < 1u || level > 9u) return 0;
        byte = (unsigned char)('0' + level);
        break;
    default: return 0;
    }
    saved = instance->port.enter_critical(instance->port.user);
    /* A successful return means accepted; a later STOP can still discard it. */
    if (instance->input.stop_pending && command != MF_COMMAND_STOP) {
        instance->port.exit_critical(instance->port.user, saved);
        return 0;
    }
    if (command != MF_COMMAND_STOP &&
        (uint8_t)((instance->input.head + 1u) % MF_INPUT_CAPACITY) == instance->input.tail) {
        MF_Input_Push(&instance->input, byte);
        instance->port.exit_critical(instance->port.user, saved);
        return 0;
    }
    MF_Input_Push(&instance->input, byte);
    instance->port.exit_critical(instance->port.user, saved);
    return 1;
}

void MF_Stop(MF_Instance *instance)
{
    (void)MF_Command(instance, MF_COMMAND_STOP, 0u);
}

int MF_GetStatus(MF_Instance *instance, MF_Status *status)
{
    uint32_t saved;
    if (!instance || !instance->initialized || !status) return 0;
    saved = instance->port.enter_critical(instance->port.user);
    status->state = instance->controller.state;
    status->target = instance->controller.target;
    status->output = MF_Controller_Output(&instance->controller);
    status->level = instance->controller.level;
    status->duty_percent = MF_Controller_DutyPercent(&instance->controller);
    status->rx_errors = instance->input.error_count;
    status->rx_overflows = instance->input.overflow_count;
    status->stop_pending = instance->input.stop_pending;
    instance->port.exit_critical(instance->port.user, saved);
    return 1;
}

void MF_OnByteISR(MF_Instance *instance, unsigned char byte)
{
    if (instance && instance->initialized) MF_Input_Push(&instance->input, byte);
}

void MF_OnRxErrorISR(MF_Instance *instance)
{
    if (instance && instance->initialized) MF_Input_Fault(&instance->input);
}
