#include "board.h"
#include "device_driver.h"

static MF_Instance *receiver;

static uint32_t EnterCritical(void *user)
{
    uint32_t saved;
    (void)user;
    saved = __get_PRIMASK();
    __disable_irq();
    return saved;
}

static void ExitCritical(void *user, uint32_t saved)
{
    (void)user;
    __set_PRIMASK(saved);
}

static uint32_t Now(void *user)
{
    (void)user;
    return Timebase_Now();
}

static uint8_t Pressed(void *user)
{
    (void)user;
    return Key_IsPressed();
}

static void Apply(void *user, MF_Direction direction, uint8_t percent)
{
    (void)user;
    MotorDriver_Apply(direction, percent);
}

void Board_Init(void)
{
    SCB->CPACR |= (0xFu << 20);
    __DSB();
    __ISB();
    Clock_Init();
    Key_Init();
    MotorDriver_Init();
    Uart2_Init(115200u);
    Timebase_Init();
}

MF_Port Board_MotorPort(void)
{
    MF_Port port = {0, EnterCritical, ExitCritical, Now, Pressed, Apply};
    return port;
}

void Board_StartReception(MF_Instance *instance)
{
    receiver = instance;
    Uart2_RX_Interrupt_Enable();
}

void USART2_IRQHandler(void)
{
    uint32_t status = USART2->SR;
    const uint32_t errors = USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE;
    if ((status & (USART_SR_RXNE | errors)) != 0u) {
        unsigned char received = (unsigned char)USART2->DR;
        if ((status & errors) != 0u) MF_OnRxErrorISR(receiver);
        else if ((status & USART_SR_RXNE) != 0u) MF_OnByteISR(receiver, received);
    }
}

void _Invalid_ISR(void)
{
    GPIOA->BSRR = 3u << 16;
    GPIOA->MODER = (GPIOA->MODER & ~0xFu) | 0x5u;
    __disable_irq();
    for (;;) { }
}
