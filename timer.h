#define TIM2_TICK		20						// usec
#define TIM2_FREQ		(1000000. / TIM2_TICK)	// Hz
#define TIM2_1ms_Pls	(TIM2_FREQ / 1000.)		// 1ms에 생성하는 pulse 수
#define TIM2_MAX		(0xFFFFFFFF)

#define TIM4_TICK		TIM2_TICK               // usec
#define TIM4_FREQ		(1000000. / TIM4_TICK)  // 
#define TIM4_1ms_Pls	(TIM4_FREQ / 1000.)
#define TIM4_MAX		(0xFFFF)

#define TIM5_FREQ		(1000000)						// Hz
#define TIM5_TICK		(1000000. / TIM5_FREQ)		// usec
#define TIM5_1ms_Pls	(TIM4_FREQ / 1000.)			// 1 ms 당 pulse 갯수
#define TIM5_MAX		(0xffffffff)