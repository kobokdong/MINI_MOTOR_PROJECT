#include "board.h"

static MF_Instance motor;

void Main(void)
{
    MF_Config config = MF_DefaultConfig();
    MF_Port port;

    Board_Init();
    port = Board_MotorPort();

    /* Application settings: keep the original control timing by default. */
    config.reverse_delay_ms = 1000u;
    config.default_level = 1u;

    if (!MF_Init(&motor, &config, &port)) {
        /* Board_Init already holds PA0/PA1 LOW. Never enable RX on init failure. */
        for (;;) { }
    }
    Board_StartReception(&motor);
    for (;;) {
        MF_Process(&motor);
    }
}
