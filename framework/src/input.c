#include "input.h"

void MF_Input_Init(MF_InputBuffer *input)
{
    input->head = 0u;
    input->tail = 0u;
    input->stop_pending = 0u;
    input->overflow_count = 0u;
    input->error_count = 0u;
}

void MF_Input_Discard(MF_InputBuffer *input)
{
    input->tail = input->head;
}

void MF_Input_Push(MF_InputBuffer *input, unsigned char byte)
{
    uint8_t next;
    /* STOP has an independent latch, even when the queue is full. */
    if (byte == 'S' || byte == 's' || byte == '0') {
        input->stop_pending = 1u;
        return;
    }
    if (input->stop_pending) return;
    /* Ignore line endings/invalid characters without occupying the queue. */
    if (!(byte == 'F' || byte == 'f' || byte == 'R' || byte == 'r' ||
          (byte >= '1' && byte <= '9'))) return;
    next = (uint8_t)((input->head + 1u) % MF_INPUT_CAPACITY);
    if (next == input->tail) {
        input->overflow_count++;
        input->stop_pending = 1u;
        return;
    }
    input->bytes[input->head] = byte;
    input->head = next;
}

void MF_Input_Fault(MF_InputBuffer *input)
{
    input->error_count++;
    input->stop_pending = 1u;
}

uint8_t MF_Input_TakeStop(MF_InputBuffer *input)
{
    if (!input->stop_pending) return 0u;
    MF_Input_Discard(input);
    input->stop_pending = 0u;
    return 1u;
}

uint8_t MF_Input_Pop(MF_InputBuffer *input, unsigned char *byte)
{
    if (input->tail == input->head) return 0u;
    *byte = input->bytes[input->tail];
    input->tail = (uint8_t)((input->tail + 1u) % MF_INPUT_CAPACITY);
    return 1u;
}
