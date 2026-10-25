#ifndef MF_BUTTON_H
#define MF_BUTTON_H
#include <stdint.h>
#define MF_BUTTON_DEBOUNCE_MS 20u
#define MF_BUTTON_SHORT_MS 900u
#define MF_BUTTON_LONG_RELEASE_MS 2100u
#define MF_BUTTON_HOLD_STOP_MS 3000u

typedef enum { MF_BUTTON_NONE, MF_BUTTON_SHORT, MF_BUTTON_LONG } MF_ButtonEvent;
typedef struct {
    uint32_t changed_at;
    uint32_t pressed_at;
    uint32_t debounce_ms;
    uint32_t short_ms;
    uint32_t long_release_ms;
    uint32_t hold_stop_ms;
    uint8_t candidate;
    uint8_t stable;
    uint8_t long_sent;
    uint8_t suppressed;
} MF_ButtonState;

void MF_Button_Init(MF_ButtonState *button, uint8_t pressed, uint32_t now);
void MF_Button_Cancel(MF_ButtonState *button);
MF_ButtonEvent MF_Button_Update(MF_ButtonState *button, uint8_t pressed, uint32_t now);
#endif
