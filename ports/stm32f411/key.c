#include "device_driver.h"

void Key_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    GPIOC->MODER &= ~(3u << 26);
    /* Preserve the original no-internal-pull configuration.
     * PC13 needs a board/external pull-up for active-low operation. */
    GPIOC->PUPDR &= ~(3u << 26);
}

uint8_t Key_IsPressed(void)
{
    return (GPIOC->IDR & (1u << 13)) == 0u;
}
