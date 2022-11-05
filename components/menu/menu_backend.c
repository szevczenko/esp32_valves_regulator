#include "stdint.h"
#include "stdarg.h"
#include <stdbool.h>

#include "config.h"
#include "menu.h"
#include "menu_drv.h"
#include "but.h"
#include "freertos/semphr.h"
#include "menu_param.h"
#include "wifidrv.h"
#include "cmd_client.h"
#include "start_menu.h"

#define MODULE_NAME    "[M BACK] "
#define DEBUG_LVL      PRINT_INFO

#if CONFIG_DEBUG_MENU_BACKEND
#define LOG(_lvl, ...) \
    debug_printf(DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__)
#else
#define LOG(PRINT_INFO, ...)
#endif

typedef enum
{
    STATE_INIT,
    STATE_IDLE,
    STATE_START,
    STATE_MENU_PARAMETERS,
    STATE_ERROR_CHECK,
    STATE_EMERGENCY_DISABLE,
    STATE_EMERGENCY_DISABLE_EXIT,
    STATE_TOP,
} state_backend_t;

typedef struct
{
    state_backend_t state;
    bool error_flag;
    char *error_msg;
    uint32_t get_data_cnt;

    bool menu_start_is_active;
    bool menu_param_is_active;
    bool emergency_msg_sended;
    bool emergency_exit_msg_sended;
    bool emergensy_req;

    bool send_all_data;
    struct menu_data sended_data;
} menu_start_context_t;

static menu_start_context_t ctx;

static char *state_name[] =
{
    [STATE_INIT] = "STATE_INIT",
    [STATE_IDLE] = "STATE_IDLE",
    [STATE_START] = "STATE_START",
    [STATE_MENU_PARAMETERS] = "STATE_MENU_PARAMETERS",
    [STATE_ERROR_CHECK] = "STATE_ERROR_CHECK",
    [STATE_EMERGENCY_DISABLE] = "STATE_EMERGENCY_DISABLE",
    [STATE_EMERGENCY_DISABLE_EXIT] = "STATE_EMERGENCY_DISABLE_EXIT"
};

static void change_state(state_backend_t new_state)
{
    debug_function_name(__func__);
    if (ctx.state < STATE_TOP)
    {
        if (ctx.state != new_state)
        {
            LOG(PRINT_INFO, "Backend menu %s", state_name[new_state]);
            ctx.state = new_state;
        }
    }
}

static void _enter_emergency(void)
{
    if (ctx.state != STATE_EMERGENCY_DISABLE)
    {
        LOG(PRINT_INFO, "%s %s", __func__, state_name[ctx.state]);
        change_state(STATE_EMERGENCY_DISABLE);
        ctx.emergency_msg_sended = false;
        ctx.emergency_exit_msg_sended = false;
        menuDrvEnterEmergencyDisable();
    }
}

static void _send_emergency_msg(void)
{
    if (ctx.emergency_msg_sended)
    {
        return;
    }

    bool ret =
        (cmdClientSetValue(MENU_EMERGENCY_DISABLE, 1, 2000) > 0) 
        && (cmdClientSetValue(MENU_VALVE_1_STATE, 0, 2000) > 0) 
        && (cmdClientSetValue(MENU_VALVE_2_STATE, 0, 2000) > 0)
        && (cmdClientSetValue(MENU_VALVE_3_STATE, 0, 2000) > 0)
        && (cmdClientSetValue(MENU_VALVE_4_STATE, 0, 2000) > 0)
        && (cmdClientSetValue(MENU_VALVE_5_STATE, 0, 2000) > 0)
        && (cmdClientSetValue(MENU_VALVE_6_STATE, 0, 2000) > 0)
        && (cmdClientSetValue(MENU_VALVE_7_STATE, 0, 2000) > 0);

    LOG(PRINT_INFO, "%s %d", __func__, ret);
    if (ret)
    {
        ctx.emergency_msg_sended = true;
        menuStartReset();
    }
}

static void _check_emergency_disable(void)
{
    if (ctx.emergensy_req)
    {
        _enter_emergency();
    }
}

static void backend_init_state(void)
{
    change_state(STATE_IDLE);
}

static void backend_idle(void)
{
    if (ctx.menu_param_is_active)
    {
        change_state(STATE_MENU_PARAMETERS);
        return;
    }

    if (ctx.menu_start_is_active)
    {
        ctx.send_all_data = true;
        change_state(STATE_START);
        return;
    }

    osDelay(50);
}

static bool _check_error(void)
{
    cmdClientGetValue(MENU_MACHINE_ERRORS, NULL, 2000);
    uint32_t errors = menuGetValue(MENU_MACHINE_ERRORS);

    if (errors > 0)
    {
        for (uint8_t i = 0; i < ERROR_TOP; i++)
        {
            if (errors & (1 << i))
            {
                menuStartSetError(i);
            }
        }

        return true;
    }

    return false;
}

static void backent_start(void)
{
    if (ctx.get_data_cnt % 20 == 0)
    {
        bool errors = _check_error() > 0;
        if (errors)
        {
            LOG(PRINT_INFO, "Error detected on machine");
        }
        else
        {
            menuStartResetError();
            LOG(PRINT_DEBUG, "No error");
        }
    }

    ctx.get_data_cnt++;

    if (ctx.menu_param_is_active)
    {
        change_state(STATE_MENU_PARAMETERS);
        return;
    }

    if (!ctx.menu_start_is_active)
    {
        change_state(STATE_IDLE);
        return;
    }

    struct menu_data *data = menuStartGetData();

    if (memcmp(&ctx.sended_data, data, sizeof(ctx.sended_data)) != 0)
    {
        ctx.send_all_data = true;
    }

    if (ctx.send_all_data)
    {
        uint8_t pass_counter = 0;

        for (int i = 0; i < CFG_VALVE_CNT; i++)
        {
            if (cmdClientSetValue(MENU_VALVE_1_STATE + i, data->valve[i].state, 1000) == TRUE)
            {
                pass_counter++;
            }
        }

        if (pass_counter == CFG_VALVE_CNT)
        {
            ctx.send_all_data = false;
            for (int i = 0; i < CFG_VALVE_CNT; i++)
            {
                ctx.sended_data.valve[i].state = data->valve[i].state;
            }
        }
    }

    // if (data->motor_value != ctx.sended_data.motor_value)
    // {
    //     if (cmdClientSetValue(MENU_MOTOR, data->motor_value, 1000) == TRUE)
    //     {
    //         ctx.sended_data.motor_value = data->motor_value;
    //     }
    // }

    osDelay(10);
}

static void backend_menu_parameters(void)
{
    if (!ctx.menu_param_is_active)
    {
        change_state(STATE_IDLE);
        return;
    }

    cmdClientGetValue(MENU_VOLTAGE_ACCUM, NULL, 2000);
    osDelay(50);
}

static void backend_error_check(void)
{
    change_state(STATE_IDLE);
}

static void backend_emergency_disable_state(void)
{
    _send_emergency_msg();
    if (!ctx.emergensy_req)
    {
        LOG(PRINT_INFO, "%s exit", __func__);
        menuDrvExitEmergencyDisable();
        change_state(STATE_EMERGENCY_DISABLE_EXIT);
    }

    osDelay(50);
}

static void backend_emergency_disable_exit(void)
{
    if (!ctx.emergency_exit_msg_sended)
    {
        int ret = cmdClientSetValue(MENU_EMERGENCY_DISABLE, 0, 2000);
        LOG(PRINT_INFO, "%s %d", __func__, ret);
        if (ret > 0)
        {
            ctx.emergency_exit_msg_sended = true;
        }

        osDelay(50);
    }
    else
    {
        change_state(STATE_IDLE);
    }
}

void backendEnterMenuParameters(void)
{
    ctx.menu_param_is_active = true;
}

void backendExitMenuParameters(void)
{
    ctx.menu_param_is_active = false;
}

void backendEnterMenuStart(void)
{
    ctx.menu_start_is_active = true;
}

void backendExitMenuStart(void)
{
    ctx.menu_start_is_active = false;
}

void backendToggleEmergencyDisable(void)
{
    if (ctx.emergensy_req)
    {
        ctx.emergensy_req = false;
    }
    else
    {
        if (wifiDrvIsConnected() && cmdClientIsConnected())
        {
            ctx.emergensy_req = true;
        }
    }
}

static void menu_task(void *arg)
{
    while (1)
    {
        _check_emergency_disable();

        switch (ctx.state)
        {
        case STATE_INIT:
            backend_init_state();
            break;

        case STATE_IDLE:
            backend_idle();
            break;

        case STATE_START:
            backent_start();
            break;

        case STATE_MENU_PARAMETERS:
            backend_menu_parameters();
            break;

        case STATE_ERROR_CHECK:
            backend_error_check();
            break;

        case STATE_EMERGENCY_DISABLE:
            backend_emergency_disable_state();
            break;

        case STATE_EMERGENCY_DISABLE_EXIT:
            backend_emergency_disable_exit();
            break;

        default:
            ctx.state = STATE_IDLE;
            break;
        }
    }
}

void menuBackendInit(void)
{
    xTaskCreate(menu_task, "menu_back", 4096, NULL, 5, NULL);
}

bool backendIsConnected(void)
{
    debug_function_name(__func__);
    if (!wifiDrvIsConnected())
    {
        LOG(PRINT_INFO, "START_MENU: WiFi not connected");
        return false;
    }

    if (!cmdClientIsConnected())
    {
        LOG(PRINT_INFO, "START_MENU: Client not connected");
        return false;
    }

    return true;
}

bool backendIsEmergencyDisable(void)
{
    return ctx.state == STATE_EMERGENCY_DISABLE;
}
