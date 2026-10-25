#include "device_driver.h"

#define PWM_COUNTER_HZ 1000000u
#define PWM_FREQUENCY_HZ 10000u
#define PWM_PERIOD_TICKS (PWM_COUNTER_HZ / PWM_FREQUENCY_HZ)
static MF_Direction applied_direction;
static uint8_t applied_percent;

void MotorDriver_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    (void)RCC->APB1ENR;

    GPIOA->BSRR = 3u << 16;
    GPIOA->MODER = (GPIOA->MODER & ~0xFu) | 0x5u;
    GPIOA->OTYPER &= ~3u;
    GPIOA->PUPDR &= ~0xFu;
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~0xFFu) | 0x22u;

    TIM5->CR1 = 0u;
    TIM5->PSC = TIMXCLK / PWM_COUNTER_HZ - 1u;
    TIM5->ARR = PWM_PERIOD_TICKS - 1u;
    TIM5->CCR1 = 0u;
    TIM5->CCR2 = 0u;
    /* PWM mode 1, preload on both channels, up counting. */
    TIM5->CCMR1 = (6u << 4) | TIM_CCMR1_OC1PE |
                  (6u << 12) | TIM_CCMR1_OC2PE;
    TIM5->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM5->CR1 = TIM_CR1_ARPE;
    TIM5->EGR = TIM_EGR_UG;
    TIM5->SR = 0u;
    TIM5->CR1 |= TIM_CR1_CEN;

    applied_direction = MF_DIR_NONE;
    applied_percent = 0u;
}

void MotorDriver_Apply(MF_Direction direction, uint8_t percent)
{
    uint32_t ticks;
    if (direction != MF_DIR_CW && direction != MF_DIR_CCW) direction = MF_DIR_NONE;
    if (percent > 100u) percent = 100u;
    if (direction == applied_direction &&
        (direction == MF_DIR_NONE || percent == applied_percent)) return;

    if (direction != applied_direction) {
        /* Disconnect both PWM pins first; both GPIO output latches are LOW. */
        GPIOA->BSRR = 3u << 16;
        GPIOA->MODER = (GPIOA->MODER & ~0xFu) | 0x5u;
    }

    if (direction != MF_DIR_NONE) {
        ticks = PWM_PERIOD_TICKS * percent / 100u;
        TIM5->CCR1 = ticks;
        TIM5->CCR2 = ticks;
        if (direction != applied_direction) {
            TIM5->EGR = TIM_EGR_UG;
            /* CW: PA1 PWM / PA0 LOW. CCW: PA0 PWM / PA1 LOW. */
            GPIOA->MODER = (GPIOA->MODER & ~0xFu) |
                          (direction == MF_DIR_CW ? 0x9u : 0x6u);
        }
    }
    applied_direction = direction;
    applied_percent = percent;
}
