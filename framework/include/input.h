#ifndef MF_INPUT_H
#define MF_INPUT_H
#include <stdint.h>
#define MF_INPUT_CAPACITY 32u

typedef struct {
    volatile unsigned char bytes[MF_INPUT_CAPACITY];
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t stop_pending;
    volatile uint32_t overflow_count;
    volatile uint32_t error_count;
} MF_InputBuffer;

/* Push/Fault are called by the serialized input ISR.
 * Pop/TakeStop/Discard are called with interrupts briefly masked by main. */
void MF_Input_Init(MF_InputBuffer *input);
void MF_Input_Push(MF_InputBuffer *input, unsigned char byte);
void MF_Input_Fault(MF_InputBuffer *input);
uint8_t MF_Input_TakeStop(MF_InputBuffer *input);
uint8_t MF_Input_Pop(MF_InputBuffer *input, unsigned char *byte);
void MF_Input_Discard(MF_InputBuffer *input);
#endif
