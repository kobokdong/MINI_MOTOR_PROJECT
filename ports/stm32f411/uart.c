#include "device_driver.h"

void Uart2_Init(unsigned int baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;
    GPIOA->MODER = (GPIOA->MODER & ~(0xFu << 4)) | (0xAu << 4);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFFu << 8)) | (0x77u << 8);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(0xFu << 4)) | (0x5u << 4);
    USART2->CR1 = 0u;
    USART2->BRR = (PCLK1 + baud / 2u) / baud;
    USART2->CR2 = 0u;
    USART2->CR3 = 0u;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void Uart2_RX_Interrupt_Enable(void)
{
    volatile uint32_t discard;
    discard = USART2->SR;
    discard = USART2->DR;
    (void)discard;
    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_SetPriority(USART2_IRQn, 1u);
    USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_PEIE;
    USART2->CR3 |= USART_CR3_EIE;
    NVIC_EnableIRQ(USART2_IRQn);
}
