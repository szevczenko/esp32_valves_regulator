#include <stdint.h>
#include "measure.h"
#include "error_valve.h"
#include "menu.h"
#include "math.h"
#include "menu_param.h"

#include "cmd_server.h"
#include "server_controller.h"

#define MODULE_NAME    "[Error] "
#define DEBUG_LVL      PRINT_INFO

#if CONFIG_DEBUG_ERROR_SIEWNIK
#define LOG(_lvl, ...) \
    debug_printf(DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__)
#else
#define LOG(PRINT_INFO, ...)
#endif

typedef enum
{
    STATE_INIT,
    STATE_IDLE,
    STATE_WORKING,
    STATE_WAIT_RESET_ERROR,
    STATE_TOP,
} state_t;

struct error_siewnik_ctx
{
    state_t state;
    bool is_error_reset;
};

static struct error_siewnik_ctx ctx;

static char *state_name[] =
{
    [STATE_INIT] = "STATE_INIT",
    [STATE_IDLE] = "STATE_IDLE",
    [STATE_WORKING] = "STATE_WORKING",
    [STATE_WAIT_RESET_ERROR] = "STATE_WAIT_RESET_ERROR",
};

static void _change_state(state_t new_state)
{
    if (ctx.state < STATE_TOP)
    {
        if (ctx.state != new_state)
        {
            LOG(PRINT_INFO, "state %s", state_name[new_state]);
        }

        ctx.state = new_state;
    }
}

static void _reset_error(void)
{
    ctx.is_error_reset = false;
}

static void _state_init(void)
{
    _change_state(STATE_IDLE);
}

static void _state_idle(void)
{
    if (menuGetValue(MENU_START_SYSTEM))
    {
        _change_state(STATE_WORKING);
    }
}

static void _state_working(void)
{
    if (menuGetValue(MENU_START_SYSTEM) == 0)
    {
        _change_state(STATE_IDLE);
    }
}

// static void _state_error_temperature(void)
// {
//     if (srvrConrollerSetError(ERROR_OVER_TEMPERATURE))
//     {
//         _change_state(STATE_WAIT_RESET_ERROR);
//     }
//     else
//     {
//         _reset_error();
//         _change_state(STATE_IDLE);
//     }
// }

static void _state_wait_reset_error(void)
{
    if (ctx.is_error_reset)
    {
        _reset_error();
        _change_state(STATE_IDLE);
    }
}

static void _error_task(void *arg)
{
    //static uint32_t error_event_timer;
    while (1)
    {
        switch (ctx.state)
        {
        case STATE_INIT:
            _state_init();
            break;

        case STATE_IDLE:
            _state_idle();
            break;

        case STATE_WORKING:
            _state_working();
            break;

        case STATE_WAIT_RESET_ERROR:
            _state_wait_reset_error();
            break;

        default:
            ctx.state = STATE_INIT;
            break;
        }

        vTaskDelay(MS2ST(200));
    } //error_event_timer
}

void errorStart(void)
{
    xTaskCreate(_error_task, "_error_task", 4096, NULL, NORMALPRIO, NULL);
}

void errorReset(void)
{
    ctx.is_error_reset = true;
}
