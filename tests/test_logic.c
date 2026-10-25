#include "controller.h"
#include "button.h"
#include "input.h"
#include <assert.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(condition) do { checks++; assert(condition); } while (0)

static void test_commands(void)
{
    MF_ControllerState m;
    MF_Controller_Init(&m);
    CHECK(m.state == MF_STATE_STOPPED && MF_Controller_Output(&m) == MF_DIR_NONE);
    CHECK(MF_Controller_DutyPercent(&m) == 50u);
    MF_Controller_Command(&m, 'f', 0u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CW);
    MF_Controller_Command(&m, '9', 1u);
    CHECK(MF_Controller_DutyPercent(&m) == 90u && MF_Controller_Output(&m) == MF_DIR_CW);
    MF_Controller_Command(&m, 'x', 2u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CW);
    MF_Controller_Command(&m, 'R', 100u);
    CHECK(m.state == MF_STATE_REVERSING && MF_Controller_Output(&m) == MF_DIR_NONE);
    MF_Controller_Update(&m, 1099u);
    CHECK(m.state == MF_STATE_REVERSING);
    MF_Controller_Update(&m, 1100u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CCW);
    MF_Controller_Command(&m, 's', 1200u);
    MF_Controller_Update(&m, 5000u);
    CHECK(m.state == MF_STATE_STOPPED && m.target == MF_DIR_NONE);
    MF_Controller_Command(&m, '5', 5001u);
    CHECK(m.state == MF_STATE_STOPPED && MF_Controller_DutyPercent(&m) == 70u);
}

static void test_cancel_and_dead_time(void)
{
    MF_ControllerState m;
    MF_Controller_Init(&m);
    MF_Controller_Command(&m, 'F', 0u);
    MF_Controller_Command(&m, 'R', 100u);
    MF_Controller_Stop(&m, 200u);
    MF_Controller_Update(&m, 5000u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_NONE);
    MF_Controller_Command(&m, 'R', 201u);
    CHECK(m.state == MF_STATE_REVERSING);
    MF_Controller_Update(&m, 1099u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_NONE);
    MF_Controller_Update(&m, 1100u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CCW);
    /* Stop/reissue opposite direction cannot bypass dead time. */
    MF_Controller_Stop(&m, 1200u);
    MF_Controller_Command(&m, 'F', 1201u);
    CHECK(m.state == MF_STATE_REVERSING);
    /* Resuming the same physical direction needs no reversal delay. */
    MF_Controller_Command(&m, 'R', 1202u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CCW);
    MF_Controller_Command(&m, '0', 1203u);
    CHECK(m.state == MF_STATE_STOPPED);
}

static void test_short_and_wrap(void)
{
    MF_ControllerState m;
    MF_Controller_Init(&m);
    MF_Controller_ShortPress(&m, 1u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CW);
    MF_Controller_ShortPress(&m, UINT32_MAX - 499u);
    CHECK(m.state == MF_STATE_REVERSING);
    MF_Controller_ShortPress(&m, UINT32_MAX - 400u);
    CHECK(m.target == MF_DIR_CCW); /* Repeated short presses ignored while waiting. */
    MF_Controller_Update(&m, 499u);
    CHECK(m.state == MF_STATE_REVERSING);
    MF_Controller_Update(&m, 500u);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CCW);
}

static MF_ButtonEvent tick(MF_ButtonState *b, unsigned pressed, uint32_t time)
{
    return MF_Button_Update(b, (uint8_t)pressed, time);
}

static void test_button(void)
{
    MF_ButtonState b;
    MF_Button_Init(&b, 0u, 0u);
    CHECK(tick(&b,1,1) == MF_BUTTON_NONE);
    CHECK(tick(&b,0,5) == MF_BUTTON_NONE); /* Bounce never becomes a press. */
    CHECK(tick(&b,0,25) == MF_BUTTON_NONE);
    tick(&b,1,100); tick(&b,1,120);
    tick(&b,0,200);
    CHECK(tick(&b,0,220) == MF_BUTTON_SHORT);
    CHECK(tick(&b,0,221) == MF_BUTTON_NONE);

    tick(&b,1,300); tick(&b,1,320);
    tick(&b,0,2420);
    CHECK(tick(&b,0,2440) == MF_BUTTON_LONG); /* A later long press isn't the first duration. */

    tick(&b,1,2500); tick(&b,1,2520);
    CHECK(tick(&b,1,5519) == MF_BUTTON_NONE);
    CHECK(tick(&b,1,5520) == MF_BUTTON_LONG);
    CHECK(tick(&b,1,5521) == MF_BUTTON_NONE);
    tick(&b,0,5600);
    CHECK(tick(&b,0,5620) == MF_BUTTON_NONE); /* No second event on release. */

    tick(&b,1,6000); tick(&b,1,6020);
    tick(&b,0,7000);
    CHECK(tick(&b,0,7020) == MF_BUTTON_NONE); /* Middle duration remains ignored. */

    tick(&b,1,8000); tick(&b,1,8020);
    MF_Button_Cancel(&b);
    tick(&b,0,8100);
    CHECK(tick(&b,0,8120) == MF_BUTTON_NONE);
    tick(&b,1,8200); tick(&b,1,8220);
    tick(&b,0,8300);
    CHECK(tick(&b,0,8320) == MF_BUTTON_SHORT);

    MF_Button_Init(&b, 1u, 0u);
    CHECK(tick(&b,1,4000) == MF_BUTTON_NONE);
    tick(&b,0,4001);
    CHECK(tick(&b,0,4021) == MF_BUTTON_NONE); /* Held during boot is suppressed. */
}

static void test_button_boundaries(void)
{
    MF_ButtonState b;
    MF_Button_Init(&b,0,0);
    tick(&b,1,0); tick(&b,1,20);
    tick(&b,0,900);
    CHECK(tick(&b,0,920) == MF_BUTTON_SHORT); /* Exactly 900 ms stable-to-stable. */
    tick(&b,1,1000); tick(&b,1,1020);
    tick(&b,0,3100);
    CHECK(tick(&b,0,3120) == MF_BUTTON_LONG); /* Exactly 2100 ms. */
    MF_Button_Init(&b,0,UINT32_MAX - 100u);
    tick(&b,1,UINT32_MAX - 80u); tick(&b,1,UINT32_MAX - 60u);
    tick(&b,0,40u);
    CHECK(tick(&b,0,60u) == MF_BUTTON_SHORT);
}

static void test_input(void)
{
    MF_InputBuffer q;
    unsigned char c;
    MF_Input_Init(&q);
    MF_Input_Push(&q,'F'); MF_Input_Push(&q,'5');
    CHECK(MF_Input_Pop(&q,&c) && c == 'F');
    CHECK(MF_Input_Pop(&q,&c) && c == '5');
    CHECK(!MF_Input_Pop(&q,&c));
    MF_Input_Push(&q,'\n'); MF_Input_Push(&q,'?');
    CHECK(!MF_Input_Pop(&q,&c));
    for (unsigned i=0;i<MF_INPUT_CAPACITY-1;i++) MF_Input_Push(&q,'R');
    MF_Input_Push(&q,'S');
    CHECK(MF_Input_TakeStop(&q));
    CHECK(!MF_Input_Pop(&q,&c));
    for (unsigned i=0;i<MF_INPUT_CAPACITY;i++) MF_Input_Push(&q,'F');
    CHECK(q.overflow_count == 1u && MF_Input_TakeStop(&q));
    MF_Input_Push(&q,'R'); MF_Input_Fault(&q);
    CHECK(q.error_count == 1u && MF_Input_TakeStop(&q));
    CHECK(!MF_Input_Pop(&q,&c));
    MF_Input_Push(&q,'0'); MF_Input_Push(&q,'F');
    CHECK(MF_Input_TakeStop(&q) && !MF_Input_Pop(&q,&c));
    for (unsigned i=0;i<100;i++) {
        MF_Input_Push(&q,'5');
        CHECK(MF_Input_Pop(&q,&c) && c == '5');
    }
}

static void test_stop_integration(void)
{
    MF_ControllerState m;
    MF_InputBuffer q;
    unsigned char c;
    MF_Controller_Init(&m);
    MF_Input_Init(&q);
    MF_Controller_Command(&m,'F',0);
    MF_Controller_Command(&m,'R',10);
    MF_Input_Push(&q,'F'); MF_Input_Push(&q,'S'); MF_Input_Push(&q,'R');
    if (MF_Input_TakeStop(&q)) MF_Controller_Stop(&m,20);
    CHECK(!MF_Input_Pop(&q,&c));
    MF_Controller_Update(&m,2000);
    CHECK(m.state == MF_STATE_STOPPED && MF_Controller_Output(&m) == MF_DIR_NONE);
    /* A fresh command after stop is consumed can start again. */
    MF_Input_Push(&q,'R');
    CHECK(MF_Input_Pop(&q,&c));
    MF_Controller_Command(&m,c,2001);
    CHECK(MF_Controller_Output(&m) == MF_DIR_CCW);
}

static void test_many_sequences(void)
{
    MF_ControllerState m;
    uint32_t seed=12345u, now=0u, last_stop=0u;
    MF_Direction previous=MF_DIR_NONE, last_run=MF_DIR_NONE;
    const unsigned char commands[]={'F','R','s','0','1','9','x'};
    MF_Controller_Init(&m);
    for (unsigned i=0;i<10000;i++) {
        seed=seed*1664525u+1013904223u;
        now += (seed >> 24);
        if ((seed & 7u)==0u) MF_Controller_ShortPress(&m,now);
        else MF_Controller_Command(&m,commands[(seed >> 8)%7u],now);
        MF_Controller_Update(&m,now);
        MF_Direction output=MF_Controller_Output(&m);
        CHECK(m.state >= MF_STATE_STOPPED && m.state <= MF_STATE_REVERSING);
        CHECK(m.level >= 1u && m.level <= 9u);
        if (previous != MF_DIR_NONE && output == MF_DIR_NONE) last_stop=now;
        if (output != MF_DIR_NONE && last_run != MF_DIR_NONE && output != last_run) {
            CHECK((uint32_t)(now-last_stop) >= MF_DEFAULT_REVERSE_MS);
        }
        if (output != MF_DIR_NONE) last_run=output;
        previous=output;
    }
}

int main(void)
{
    test_commands();
    test_cancel_and_dead_time();
    test_short_and_wrap();
    test_button();
    test_button_boundaries();
    test_input();
    test_stop_integration();
    test_many_sequences();
    printf("PASS: %u assertions across 8 test groups (including 10000 command steps).\n",checks);
    return 0;
}
