#include <stdbool.h>
#include "server_controller.h"
#include "menu_param.h"
#include "parse_cmd.h"
#include "cmd_server.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "error_valve.h"
#include "measure.h"

#define MODULE_NAME       "[Srvr Ctrl] "
#define DEBUG_LVL         PRINT_INFO

#if CONFIG_DEBUG_SERVER_CONTROLLER
#define LOG(_lvl, ...) \
    debug_printf(DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__)
#else
#define LOG(PRINT_INFO, ...)
#endif

#define SYSTEM_ON_PIN     15

typedef enum
{
    STATE_INIT,
    STATE_IDLE,
    STATE_WORKING,
    STATE_EMERGENCY_DISABLE,
    STATE_ERROR,
    STATE_LAST,
} state_t;

struct valve_data
{
    bool valve_on;  //Stan wysłany przez pilot
    bool state;     //Stan rzeczywisty
    int gpio;
    menuValue_t menu_value;
};

typedef struct
{
    state_t state;
    
    bool system_on;
    bool emergency_disable;
    bool errors;

    bool water_on;
    uint32_t water_volume_l;
    uint32_t water_read_vol;
    struct valve_data valves[CFG_VALVE_CNT];

    bool working_state_req;
} server_conroller_ctx;

static server_conroller_ctx ctx = 
{
    .valves = {
        {
            .gpio = 13,
            .menu_value = MENU_VALVE_1_STATE
        },
        {
            .gpio = 14,
            .menu_value = MENU_VALVE_2_STATE
        },
        {
            .gpio = 26,
            .menu_value = MENU_VALVE_3_STATE
        },
        {
            .gpio = 25,
            .menu_value = MENU_VALVE_4_STATE
        },
        {
            .gpio = 32,
            .menu_value = MENU_VALVE_5_STATE
        },
        {
            .gpio = 18,
            .menu_value = MENU_VALVE_6_STATE,
        },
        {
            .gpio = 19,
            .menu_value = MENU_VALVE_7_STATE,
        }
    }
};

static char *state_name[] =
{
    [STATE_INIT] = "STATE_INIT",
    [STATE_IDLE] = "STATE_IDLE",
    [STATE_WORKING] = "STATE_WORKING",
    [STATE_EMERGENCY_DISABLE] = "STATE_EMERGENCY_DISABLE",
    [STATE_ERROR] = "STATE_ERROR"
};

static void change_state(state_t state)
{
    if (state >= STATE_LAST)
    {
        return;
    }

    if (state != ctx.state)
    {
        LOG(PRINT_INFO, "Change state -> %s", state_name[state]);
        ctx.state = state;
    }
}

static void count_working_data(void)
{

}

static void _reset_pulse_counter(void)
{

}

static void _on_valve(struct valve_data * valve)
{
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_A, 50.00);
    gpio_set_level(valve->gpio, 1);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_A, 99.99);
    osDelay(100);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_A, 50.00);
    valve->state = 1;
}

static void _off_valve(struct valve_data * valve)
{
    gpio_set_level(valve->gpio, 0);
    valve->state = 0;
}

static void set_working_data(void)
{
    if (ctx.system_on)
    {
        gpio_set_level(SYSTEM_ON_PIN, 1);
    }
    else
    {
        gpio_set_level(SYSTEM_ON_PIN, 0);
    }

    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        if (ctx.valves[i].valve_on != ctx.valves[i].state)
        {
            if (ctx.valves[i].valve_on)
            {
                _on_valve(&ctx.valves[i]);
            }
            else
            {
                _off_valve(&ctx.valves[i]);
            }
        }
    }
}

static void _init_pwm_pin(void)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, CFG_VALVE_CURRENT_REGULATION_PIN);
    mcpwm_config_t pwm_config;

    pwm_config.frequency = 1000;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_DOWN_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_A, MCPWM_DUTY_MODE_0);
}

static void state_init(void)
{
    _init_pwm_pin();

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << SYSTEM_ON_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        io_conf.pin_bit_mask = BIT64(ctx.valves[i].gpio);
        gpio_config(&io_conf);
    }

    change_state(STATE_IDLE);
}

static void state_idle(void)
{
    ctx.working_state_req = (bool)menuGetValue(MENU_START_SYSTEM);
    ctx.emergency_disable = (bool)menuGetValue(MENU_EMERGENCY_DISABLE);

    if (ctx.emergency_disable)
    {
        change_state(STATE_EMERGENCY_DISABLE);
        return;
    }

    if (ctx.working_state_req && cmdServerIsWorking())
    {
        measure_meas_calibration_value();
        count_working_data();
        set_working_data();
        osDelay(1000);
        change_state(STATE_WORKING);
        return;
    }

    osDelay(100);
}

static void state_working(void)
{
    ctx.system_on = (bool)menuGetValue(MENU_START_SYSTEM);

    ctx.working_state_req = (bool)menuGetValue(MENU_START_SYSTEM);
    ctx.emergency_disable = (bool)menuGetValue(MENU_EMERGENCY_DISABLE);

    if (ctx.emergency_disable)
    {
        change_state(STATE_EMERGENCY_DISABLE);
        return;
    }

    if (!ctx.working_state_req || !cmdServerIsWorking())
    {
        change_state(STATE_IDLE);
        return;
    }

    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        ctx.valves[i].valve_on = menuGetValue(ctx.valves[i].menu_value);
    }

    ctx.water_volume_l = menuGetValue(MENU_WATER_VOL_ADD);

    if (!ctx.water_on && menuGetValue(MENU_ADD_WATER))
    {
        _reset_pulse_counter();
        ctx.water_read_vol = 0;
    }

    ctx.water_on = menuGetValue(MENU_ADD_WATER);

    if (ctx.water_read_vol >= ctx.water_volume_l)
    {
        menuSetValue(MENU_ADD_WATER, 0);
        ctx.water_on = false;
    }

    menuSetValue(MENU_VALVE_6_STATE, ctx.water_on);
    menuSetValue(MENU_WATER_VOL_READ, ctx.water_read_vol);

    ctx.water_read_vol++;
    
    osDelay(100);
}

static void state_emergency_disable(void)
{
    // Tą linijke usunąć jeżeli niepotrzebne wyłączenie przekaźnika w trybie STOP
    ctx.system_on = 0;
    ctx.emergency_disable = (bool)menuGetValue(MENU_EMERGENCY_DISABLE);

    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        ctx.valves[i].valve_on = false;
    }
    
    ctx.water_on = false;

    if (!ctx.emergency_disable)
    {
        change_state(STATE_IDLE);
        return;
    }

    osDelay(100);
}

static void state_error(void)
{
    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        ctx.valves[i].valve_on = false;
    }
    
    ctx.water_on = false;

    ctx.errors = (bool)menuGetValue(MENU_MACHINE_ERRORS);

    if (!ctx.errors)
    {
        errorReset();
        change_state(STATE_IDLE);
        return;
    }

    osDelay(100);
}

static void _task(void *arg)
{
    while (1)
    {
        switch (ctx.state)
        {
        case STATE_INIT:
            state_init();
            break;

        case STATE_IDLE:
            state_idle();
            break;

        case STATE_WORKING:
            state_working();
            break;

        case STATE_EMERGENCY_DISABLE:
            state_emergency_disable();
            break;

        case STATE_ERROR:
            state_error();
            break;

        default:
            change_state(STATE_IDLE);
            break;
        }

        count_working_data();
        set_working_data();
    }
}

bool srvrControllIsWorking(void)
{
    return ctx.state == STATE_WORKING;
}

bool srvrControllGetEmergencyDisable(void)
{
    return ctx.emergency_disable;
}

void srvrControllStart(void)
{
    xTaskCreate(_task, "srvrController", 4096, NULL, 10, NULL);
}

bool srvrConrollerSetError(uint16_t error_reason)
{
    if (ctx.state == STATE_WORKING)
    {
        change_state(STATE_ERROR);
        uint16_t error = (1 << error_reason);
        menuSetValue(MENU_MACHINE_ERRORS, error);
        return true;
    }

    return false;
}

bool srvrControllerErrorReset(void)
{
    if (ctx.state == STATE_ERROR)
    {
        errorReset();
        change_state(STATE_IDLE);
        return true;
    }

    return false;
}
