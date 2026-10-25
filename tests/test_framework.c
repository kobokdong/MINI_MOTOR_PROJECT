#include "motor_framework.h"
#include <assert.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(c) do { checks++; assert(c); } while(0)

typedef struct {
    uint32_t now;
    uint32_t masked;
    unsigned writes;
    uint8_t pressed;
    MF_Direction output;
    uint8_t percent;
} fake_board_t;

static uint32_t enter(void *user)
{
    fake_board_t *f=user;
    uint32_t saved=f->masked;
    f->masked=1u;
    return saved;
}
static void leave(void *user,uint32_t saved)
{
    fake_board_t *f=user;
    CHECK(f->masked==1u);
    f->masked=saved;
}
static uint32_t now_ms(void *user)
{
    fake_board_t *f=user;
    CHECK(f->masked==1u);
    return f->now;
}
static uint8_t pressed(void *user)
{
    fake_board_t *f=user;
    CHECK(f->masked==1u);
    return f->pressed;
}
static void apply(void *user,MF_Direction dir,uint8_t percent)
{
    fake_board_t *f=user;
    CHECK(f->masked==1u);
    f->output=dir;
    f->percent=percent;
    f->writes++;
}
static MF_Port port_for(fake_board_t *f)
{
    MF_Port port={f,enter,leave,now_ms,pressed,apply};
    return port;
}
static MF_Config quick_config(void)
{
    MF_Config cfg=MF_DefaultConfig();
    cfg.reverse_delay_ms=250u;
    cfg.debounce_ms=5u;
    cfg.short_press_ms=40u;
    cfg.long_release_ms=80u;
    cfg.hold_stop_ms=100u;
    cfg.default_level=3u;
    return cfg;
}
static MF_Status status(MF_Instance *m)
{
    MF_Status s;
    CHECK(MF_GetStatus(m,&s));
    return s;
}
static void step(MF_Instance *m,fake_board_t *f,uint32_t time,uint8_t key)
{
    f->now=time;
    f->pressed=key;
    MF_Process(m);
}

static void test_validation(void)
{
    fake_board_t f={0};
    MF_Instance m={0};
    MF_Config cfg=MF_DefaultConfig();
    MF_Port p=port_for(&f);
    CHECK(!MF_Init(0,&cfg,&p));
    CHECK(!MF_Init(&m,0,&p));
    CHECK(!MF_Init(&m,&cfg,0));
    p.apply_output=0;
    CHECK(!MF_Init(&m,&cfg,&p));
    p=port_for(&f);
    cfg.debounce_ms=0;
    CHECK(!MF_Init(&m,&cfg,&p));
    cfg=MF_DefaultConfig(); cfg.reverse_delay_ms=0;
    CHECK(!MF_Init(&m,&cfg,&p));
    cfg=MF_DefaultConfig(); cfg.default_level=10;
    CHECK(!MF_Init(&m,&cfg,&p));
    cfg=MF_DefaultConfig(); cfg.short_press_ms=cfg.long_release_ms;
    CHECK(!MF_Init(&m,&cfg,&p));
    cfg=MF_DefaultConfig(); cfg.hold_stop_ms=cfg.long_release_ms-1;
    CHECK(!MF_Init(&m,&cfg,&p));
    CHECK(f.writes==0);
    MF_Process(&m); MF_Process(0); MF_Stop(0);
    MF_OnByteISR(0,'S'); MF_OnRxErrorISR(0);
    CHECK(!MF_Command(&m,MF_COMMAND_CW,0));
    CHECK(!MF_GetStatus(&m,0));
}

static void test_api_and_stop(void)
{
    fake_board_t f={0};
    MF_Instance m={0};
    MF_Config cfg=quick_config();
    MF_Port p=port_for(&f);
    CHECK(MF_Init(&m,&cfg,&p));
    CHECK(f.output==MF_DIR_NONE && f.writes==1);
    CHECK(status(&m).duty_percent==60);
    step(&m,&f,1,0);
    CHECK(f.writes==1); /* Idle doesn't repeatedly invoke a hardware callback. */
    CHECK(!MF_Command(&m,MF_COMMAND_LEVEL,0));
    CHECK(!MF_Command(&m,MF_COMMAND_LEVEL,10));
    CHECK(!MF_Command(&m,(MF_CommandType)99,0));
    CHECK(MF_Command(&m,MF_COMMAND_CW,0));
    CHECK(f.output==MF_DIR_NONE); /* Submission is asynchronous. */
    step(&m,&f,10,0);
    CHECK(f.output==MF_DIR_CW && f.percent==60);
    CHECK(MF_Command(&m,MF_COMMAND_LEVEL,9));
    step(&m,&f,11,0);
    CHECK(f.percent==90);
    CHECK(MF_Command(&m,MF_COMMAND_CCW,0));
    step(&m,&f,20,0);
    CHECK(status(&m).state==MF_STATE_REVERSING && f.output==MF_DIR_NONE);
    step(&m,&f,269,0);
    CHECK(f.output==MF_DIR_NONE);
    step(&m,&f,270,0);
    CHECK(f.output==MF_DIR_CCW);
    MF_Stop(&m);
    CHECK(status(&m).stop_pending==1);
    CHECK(!MF_Command(&m,MF_COMMAND_CW,0));
    step(&m,&f,280,0);
    CHECK(f.output==MF_DIR_NONE && status(&m).target==MF_DIR_NONE);
    step(&m,&f,2000,0);
    CHECK(f.output==MF_DIR_NONE);
}

static void test_isr_priority_and_errors(void)
{
    fake_board_t f={0};
    MF_Instance m={0};
    MF_Config cfg=MF_DefaultConfig();
    MF_Port p=port_for(&f);
    CHECK(MF_Init(&m,&cfg,&p));
    MF_OnByteISR(&m,'F');
    MF_OnByteISR(&m,'S');
    MF_OnByteISR(&m,'R');
    step(&m,&f,0,0);
    step(&m,&f,1,0);
    CHECK(f.output==MF_DIR_NONE);
    MF_OnByteISR(&m,'R');
    step(&m,&f,2,0);
    CHECK(f.output==MF_DIR_CCW);
    MF_OnRxErrorISR(&m);
    step(&m,&f,3,0);
    CHECK(f.output==MF_DIR_NONE && status(&m).rx_errors==1);
    for(unsigned i=0;i<MF_INPUT_CAPACITY;i++) MF_OnByteISR(&m,'F');
    step(&m,&f,4,0);
    CHECK(status(&m).rx_overflows==1 && f.output==MF_DIR_NONE);
    for(unsigned i=0;i<MF_INPUT_CAPACITY-1;i++) CHECK(MF_Command(&m,MF_COMMAND_CW,0));
    CHECK(!MF_Command(&m,MF_COMMAND_CW,0));
    CHECK(status(&m).rx_overflows==2);
    step(&m,&f,5,0);
    CHECK(f.output==MF_DIR_NONE);
}

static void test_configured_button(void)
{
    fake_board_t f={0};
    MF_Instance m={0};
    MF_Config cfg=quick_config();
    MF_Port p=port_for(&f);
    CHECK(MF_Init(&m,&cfg,&p));
    step(&m,&f,10,1); step(&m,&f,14,1);
    CHECK(f.output==MF_DIR_NONE);
    step(&m,&f,15,1);
    step(&m,&f,30,0); step(&m,&f,35,0);
    CHECK(f.output==MF_DIR_CW); /* Custom short duration, not a fixed 900ms. */
    step(&m,&f,40,1); step(&m,&f,45,1);
    step(&m,&f,144,1);
    CHECK(f.output==MF_DIR_CW);
    step(&m,&f,145,1);
    CHECK(f.output==MF_DIR_NONE);
    step(&m,&f,150,0); step(&m,&f,155,0);
    CHECK(f.output==MF_DIR_NONE);
    step(&m,&f,160,1); step(&m,&f,165,1);
    step(&m,&f,180,0); step(&m,&f,185,0);
    CHECK(f.output==MF_DIR_CW);
    /* UART stop while a short press is being held must suppress its release. */
    step(&m,&f,190,1); step(&m,&f,195,1);
    MF_OnByteISR(&m,'0');
    step(&m,&f,196,1);
    step(&m,&f,200,0); step(&m,&f,205,0);
    CHECK(f.output==MF_DIR_NONE);
}

static void test_instances_and_critical_restore(void)
{
    fake_board_t a={0},b={0};
    MF_Instance one={0},two={0};
    MF_Config cfg=MF_DefaultConfig();
    MF_Port pa=port_for(&a),pb=port_for(&b);
    a.masked=1u; /* Port must restore preexisting interrupt-mask state. */
    CHECK(MF_Init(&one,&cfg,&pa));
    CHECK(a.masked==1u);
    CHECK(MF_Init(&two,&cfg,&pb));
    CHECK(MF_Command(&one,MF_COMMAND_CW,0));
    step(&one,&a,1,0);
    step(&two,&b,1,0);
    CHECK(a.output==MF_DIR_CW && b.output==MF_DIR_NONE && a.masked==1u && b.masked==0u);
    MF_OnByteISR(&two,'R');
    step(&two,&b,2,0);
    MF_Stop(&one); step(&one,&a,3,0);
    CHECK(a.output==MF_DIR_NONE && b.output==MF_DIR_CCW);
}

int main(void)
{
    test_validation();
    test_api_and_stop();
    test_isr_priority_and_errors();
    test_configured_button();
    test_instances_and_critical_restore();
    printf("PASS: %u framework API assertions across 5 integration groups.\n",checks);
    return 0;
}
