#include "device_driver.h"

static volatile uint32_t milliseconds;

void Timebase_Init(void)
{
    milliseconds = 0u;
    /* HCLK source, reload counts 0..LOAD, interrupt once per millisecond. */
    SysTick->CTRL = 0u;
    SysTick->LOAD = HCLK / 1000u - 1u;
    SysTick->VAL = 0u;
    NVIC_SetPriority(SysTick_IRQn, 2u);
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    milliseconds++;
}

uint32_t Timebase_Now(void)
{
    return milliseconds;
}
