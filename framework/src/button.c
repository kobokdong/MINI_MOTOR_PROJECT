#include "button.h"

void MF_Button_Init(MF_ButtonState *button, uint8_t pressed, uint32_t now)
{
    button->changed_at = now;
    button->pressed_at = now;
    button->debounce_ms = MF_BUTTON_DEBOUNCE_MS;
    button->short_ms = MF_BUTTON_SHORT_MS;
    button->long_release_ms = MF_BUTTON_LONG_RELEASE_MS;
    button->hold_stop_ms = MF_BUTTON_HOLD_STOP_MS;
    button->candidate = pressed;
    button->stable = pressed;
    button->long_sent = 0u;
    button->suppressed = pressed; /* Booting with the button held cannot start a motor. */
}

void MF_Button_Cancel(MF_ButtonState *button)
{
    button->suppressed = 1u;
    button->long_sent = 0u;
}

MF_ButtonEvent MF_Button_Update(MF_ButtonState *button, uint8_t pressed, uint32_t now)
{
    if (pressed != button->candidate) {
        button->candidate = pressed;
        button->changed_at = now;
    }
    if (button->stable != button->candidate &&
        (uint32_t)(now - button->changed_at) >= button->debounce_ms) {
        button->stable = button->candidate;
        if (button->stable) {
            button->pressed_at = now;
            button->long_sent = 0u;
        } else if (!button->suppressed && !button->long_sent) {
            uint32_t duration = now - button->pressed_at;
            if (duration >= button->long_release_ms) return MF_BUTTON_LONG;
            if (duration <= button->short_ms) return MF_BUTTON_SHORT;
        }
    }

    if (button->suppressed) {
        if (!pressed && !button->stable &&
            (uint32_t)(now - button->changed_at) >= button->debounce_ms) {
            button->suppressed = 0u;
        }
        return MF_BUTTON_NONE;
    }
    if (button->stable && button->candidate && !button->long_sent &&
        (uint32_t)(now - button->pressed_at) >= button->hold_stop_ms) {
        button->long_sent = 1u;
        return MF_BUTTON_LONG;
    }
    return MF_BUTTON_NONE;
}
