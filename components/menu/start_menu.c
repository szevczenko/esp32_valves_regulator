#include "config.h"
#include "menu.h"
#include "menu_drv.h"
#include "ssd1306.h"
#include "ssdFigure.h"
#include "menu_default.h"
#include "menu_backend.h"
#include "freertos/timers.h"

#include "wifidrv.h"
#include "cmd_client.h"
#include "fast_add.h"
#include "battery.h"
#include "buzzer.h"
#include "start_menu.h"
#include "oled.h"
#include "string.h"

#define MODULE_NAME                 "[START] "
#define DEBUG_LVL                   PRINT_INFO

#if CONFIG_DEBUG_MENU_BACKEND
#define LOG(_lvl, ...) \
    debug_printf(DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__)
#else
#define LOG(PRINT_INFO, ...)
#endif

#define DEVICE_LIST_SIZE            16
#define CHANGE_MENU_TIMEOUT_MS      1500
#define POWER_SAVE_TIMEOUT_MS       30 * 1000
#define CHANGE_VALUE_DISP_OFFSET    40
#define MENU_START_OFFSET           42

typedef enum
{
    STATE_INIT,
    STATE_CHECK_WIFI,
    STATE_IDLE,
    STATE_START,
    STATE_READY,
    STATE_POWER_SAVE,
    STATE_ERROR,
    STATE_INFO,
    STATE_STOP,
    STATE_ERROR_CHECK,
    STATE_RECONNECT,
    STATE_WAIT_CONNECT,
    STATE_TOP,
} state_start_menu_t;

typedef enum
{
    SUBSTATE_MAIN,
    SUBSTATE_WATER_ADD,
    SUBSTATE_APPLY_WATER,
    SUBSTATE_WAIT_WATER_ADD,
    SUBSTATE_TOP,
} menu_substate_t;

typedef struct
{
    state_start_menu_t state;
    state_start_menu_t last_state;
    menu_substate_t substate;
    bool error_flag;
    bool exit_wait_flag;
    int error_code;
    const char *error_msg;
    const char *info_msg;
    char buff[128];
    char ap_name[64];
    uint32_t timeout_con;
    uint32_t low_silos_ckeck_timeout;
    error_type_t error_dev;

    struct menu_data data;
    TickType_t animation_timeout;
    uint8_t animation_cnt;
    TickType_t go_to_power_save_timeout;
    TickType_t low_silos_timeout;
} menu_start_context_t;

static menu_start_context_t ctx;

loadBar_t motor_bar =
{
    .x      = 40,
    .y      = 10,
    .width  = 80,
    .height = 10,
};

loadBar_t servo_bar =
{
    .x      = 40,
    .y      = 40,
    .width  = 80,
    .height = 10,
};

static char *state_name[] =
{
    [STATE_INIT] = "STATE_INIT",
    [STATE_IDLE] = "STATE_IDLE",
    [STATE_CHECK_WIFI] = "STATE_CHECK_WIFI",
    [STATE_START] = "STATE_START",
    [STATE_READY] = "STATE_READY",
    [STATE_POWER_SAVE] = "STATE_POWER_SAVE",
    [STATE_ERROR] = "STATE_ERROR",
    [STATE_INFO] = "STATE_INFO",
    [STATE_STOP] = "STATE_STOP",
    [STATE_ERROR_CHECK] = "STATE_ERROR_CHECK",
    [STATE_RECONNECT] = "STATE_RECONNECT",
    [STATE_WAIT_CONNECT] = "STATE_WAIT_CONNECT"
};

static void change_state(state_start_menu_t new_state)
{
    debug_function_name(__func__);
    if (ctx.state < STATE_TOP)
    {
        if (ctx.state != new_state)
        {
            LOG(PRINT_INFO, "Start menu %s", state_name[new_state]);
        }

        ctx.state = new_state;
    }
    else
    {
        LOG(PRINT_INFO, "change state %d", new_state);
    }
}

static void _reset_error(void)
{
    if (menuGetValue(MENU_MACHINE_ERRORS))
    {
        cmdClientSetValueWithoutResp(MENU_MACHINE_ERRORS, 0);
    }
}

static bool _is_working_state(void)
{
    if (ctx.state == STATE_READY)
    {
        return true;
    }

    return false;
}

static bool _is_power_save(void)
{
    if (ctx.state == STATE_POWER_SAVE)
    {
        return true;
    }

    return false;
}

// static void _enter_power_save(void)
// {
//  wifiDrvPowerSave(true);
// }

static void _exit_power_save(void)
{
    wifiDrvPowerSave(false);
}

static void _reset_power_save_timer(void)
{
    ctx.go_to_power_save_timeout = MS2ST(POWER_SAVE_TIMEOUT_MS) + xTaskGetTickCount();
}

static bool default_button_process(void *arg)
{
    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return false;
    }

    _reset_error();
    _reset_power_save_timer();

    if (_is_power_save())
    {
        _exit_power_save();
        change_state(STATE_READY);
    }

    if (!_is_working_state())
    {
        return false;
    }

    return true;
}

static void fast_add_callback(uint32_t value)
{

}

static void menu_button_up_callback(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    if (ctx.substate == SUBSTATE_WATER_ADD)
    {
        if (ctx.data.water_volume_l < menuGetMaxValue(MENU_WATER_VOL_ADD))
        {
            ctx.data.water_volume_l += 10; 
        }
    }
}

static void menu_button_up_time_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    if (ctx.substate == SUBSTATE_WATER_ADD)
    {
        fastProcessStart(&ctx.data.water_volume_l, menuGetMaxValue(MENU_WATER_VOL_ADD), 1, FP_PLUS_10, fast_add_callback);
    }
    else
    {
        enterMenuParameters();
    }
}

static void menu_button_up_down_pull_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    fastProcessStop(&ctx.data.water_volume_l);
}

static void menu_button_down_callback(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    if (ctx.substate == SUBSTATE_WATER_ADD)
    {
        if (ctx.data.water_volume_l >= 10)
        {
            ctx.data.water_volume_l -= 10; 
        }
    }
}

static void menu_button_down_time_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    if (ctx.substate == SUBSTATE_WATER_ADD)
    {
        fastProcessStart(&ctx.data.water_volume_l, menuGetMaxValue(MENU_WATER_VOL_ADD), 1, FP_MINUS_10, fast_add_callback);
    }
    else
    {
        enterMenuParameters();
    }
}

static void menu_button_exit_callback(void *arg)
{
    debug_function_name(__func__);
    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return;
    }
    _reset_error();
    menuExit(menu);
    ctx.exit_wait_flag = true;
}

static void menu_button_valve_5_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    ctx.data.valve[4].state = ctx.data.valve[4].state ? false : true;
}

static void menu_button_add_water_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }

    if (ctx.substate == SUBSTATE_WATER_ADD)
    {
        ctx.substate = SUBSTATE_APPLY_WATER;
    }
    else if (ctx.substate == SUBSTATE_APPLY_WATER)
    {
        backendSetWater(true);
        menuSetValue(MENU_ADD_WATER, 1);
        ctx.substate = SUBSTATE_WAIT_WATER_ADD;
    }
}

static void menu_button_add_timer_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }

    if (ctx.substate == SUBSTATE_MAIN)
    {
        ctx.substate = SUBSTATE_WATER_ADD;
    }
}

static void menu_button_valve_2_push_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }

    ctx.data.valve[1].state = ctx.data.valve[1].state ? false : true;
}

static void menu_button_valve_2_time_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }
}

static void menu_button_valve_1_push_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    ctx.data.valve[0].state = ctx.data.valve[0].state ? false : true;
}

static void menu_button_valve_1_time_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }
}

static void menu_button_valve_1_2_pull_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }

    menuDrvSaveParameters();
}

/*-------------DOWN BUTTONS------------*/

static void menu_button_valve_4_push_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }

    ctx.data.valve[3].state = ctx.data.valve[3].state ? false : true;
}

static void menu_button_valve_4_time_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }
}

static void menu_button_valve_3_push_cb(void *arg)
{
    debug_function_name(__func__);
    
    if (!default_button_process(arg))
    {
        return;
    }

    ctx.data.valve[2].state = ctx.data.valve[2].state ? false : true;
}

static void menu_button_valve_3_time_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }
}

static void menu_button_valve_3_4_pull_cb(void *arg)
{
    debug_function_name(__func__);

    if (!default_button_process(arg))
    {
        return;
    }

    menuDrvSaveParameters();
}

static void menu_button_on_off(void *arg)
{
    debug_function_name(__func__);
    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return;
    }

    _reset_error();
}

static bool menu_button_init_cb(void *arg)
{
    debug_function_name(__func__);
    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return false;
    }

    menu->button.down.fall_callback = menu_button_down_callback;
    // menu->button.down.timer_callback = menu_enter_parameters_callback;
    menu->button.down.timer_callback = menu_button_down_time_cb;
    menu->button.down.rise_callback = menu_button_up_down_pull_cb;
    // menu->button.up.timer_callback = menu_enter_parameters_callback;
    menu->button.up.timer_callback = menu_button_up_time_cb;
    menu->button.up.fall_callback = menu_button_up_callback;
    menu->button.up.rise_callback = menu_button_up_down_pull_cb;
    menu->button.enter.fall_callback = menu_button_exit_callback;
    menu->button.exit.fall_callback = menu_button_valve_5_cb;

    menu->button.up_minus.fall_callback = menu_button_valve_1_push_cb;
    menu->button.up_minus.rise_callback = menu_button_valve_1_2_pull_cb;
    menu->button.up_minus.timer_callback = menu_button_valve_1_time_cb;
    menu->button.up_plus.fall_callback = menu_button_valve_2_push_cb;
    menu->button.up_plus.rise_callback = menu_button_valve_1_2_pull_cb;
    menu->button.up_plus.timer_callback = menu_button_valve_2_time_cb;

    menu->button.down_minus.fall_callback = menu_button_valve_3_push_cb;
    menu->button.down_minus.rise_callback = menu_button_valve_3_4_pull_cb;
    menu->button.down_minus.timer_callback = menu_button_valve_3_time_cb;
    menu->button.down_plus.fall_callback = menu_button_valve_4_push_cb;
    menu->button.down_plus.rise_callback = menu_button_valve_3_4_pull_cb;
    menu->button.down_plus.timer_callback = menu_button_valve_4_time_cb;

    menu->button.motor_on.fall_callback = menu_button_add_water_cb;
    menu->button.motor_on.timer_callback = menu_button_add_timer_cb;
    menu->button.on_off.fall_callback = menu_button_on_off;
    return true;
}

static bool menu_enter_cb(void *arg)
{
    debug_function_name(__func__);
    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return false;
    }

    _exit_power_save();
    _reset_power_save_timer();

    if (!backendIsConnected())
    {
        change_state(STATE_INIT);
    }

    cmdClientSetValueWithoutResp(MENU_START_SYSTEM, 1);

    backendEnterMenuStart();

    ctx.error_flag = 0;
    return true;
}

static bool menu_exit_cb(void *arg)
{
    debug_function_name(__func__);

    backendExitMenuStart();

    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return false;
    }

    MOTOR_LED_SET_GREEN(0);
    SERVO_VIBRO_LED_SET_GREEN(0);
    return true;
}

static void menu_set_error_msg(const char *msg)
{
    ctx.error_msg = msg;
    ctx.error_flag = 1;
    change_state(STATE_ERROR_CHECK);
}

static void menu_start_init(void)
{
    oled_printFixed(2, 2 * LINE_HEIGHT, dictionary_get_string(DICT_CHECK_CONNECTION), OLED_FONT_SIZE_11);
    change_state(STATE_CHECK_WIFI);
}

static void menu_check_connection(void)
{
    if (!backendIsConnected())
    {
        change_state(STATE_RECONNECT);
        return;
    }

    bool ret = false;

    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        ctx.data.valve[i].state = menuGetValue(MENU_VALVE_1_STATE + i);
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        LOG(PRINT_INFO, "START_MENU: cmdClientGetAllValue try %d", i);
        osDelay(250);
        

        if (cmdClientSetValue(MENU_EMERGENCY_DISABLE, 0, 1000) == TRUE)
        {
            break;
        }
    }

    if (ret != TRUE)
    {
        LOG(PRINT_INFO, "%s: error get parameters", __func__);
        change_state(STATE_IDLE);
    }

    change_state(STATE_IDLE);
}

static void menu_start_idle(void)
{
    if (backendIsConnected())
    {
        cmdClientSetValueWithoutResp(MENU_START_SYSTEM, 1);
        change_state(STATE_START);
    }
    else
    {
        menuPrintfInfo("Target not connected.\nGo to DEVICES\nfor connect");
    }
}

static void menu_start_start(void)
{
    if (!backendIsConnected())
    {
        return;
    }

    change_state(STATE_READY);
}

static void _substate_main(void)
{
    static char buff_on[64];
    static char buff_off[64];

    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        sprintf(buff_on, "Valve on:  %s %s %s %s %s", 
            ctx.data.valve[0].state ? "1" : "_", 
            ctx.data.valve[1].state ? "2" : "_", 
            ctx.data.valve[2].state ? "3" : "_", 
            ctx.data.valve[3].state ? "4" : "_", 
            ctx.data.valve[4].state ? "5" : "_");

        sprintf(buff_off, "Valve off: %s %s %s %s %s", 
            !ctx.data.valve[0].state ? "1" : "_", 
            !ctx.data.valve[1].state ? "2" : "_", 
            !ctx.data.valve[2].state ? "3" : "_", 
            !ctx.data.valve[3].state ? "4" : "_", 
            !ctx.data.valve[4].state ? "5" : "_");
    }

    oled_printFixed(2, MENU_HEIGHT, buff_on, OLED_FONT_SIZE_11);
    oled_printFixed(2, MENU_HEIGHT * 2, buff_off, OLED_FONT_SIZE_11);

    if (ctx.animation_timeout < xTaskGetTickCount())
    {
        ctx.animation_cnt++;
        ctx.animation_timeout = xTaskGetTickCount() + MS2ST(100);
    }
}

static void _substate_add_water(void)
{
    static char buff_water[64];

    oled_printFixed(2, MENU_HEIGHT, "WATER ADD:", OLED_FONT_SIZE_11);
    sprintf(buff_water, "%d [l]", ctx.data.water_volume_l);
    oled_printFixed(60, MENU_HEIGHT * 2, buff_water, OLED_FONT_SIZE_11);
}

static void _substate_apply_water(void)
{
    oled_printFixed(2, MENU_HEIGHT, "Add water?", OLED_FONT_SIZE_11);
}

static void _substate_wait_water_add(void)
{
    static char buff_water[64];
    uint32_t water_added = menuGetValue(MENU_WATER_VOL_READ);
    sprintf(buff_water, "Water in tank %d [l]", water_added);
    oled_printFixed(2, MENU_HEIGHT, buff_water, OLED_FONT_SIZE_11);

    if (water_added > ctx.data.water_volume_l - 10 || !menuGetValue(MENU_ADD_WATER))
    {
        ctx.substate = SUBSTATE_MAIN;
    }
}

static void menu_start_ready(void)
{
    debug_function_name(__func__);
    if (!backendIsConnected())
    {
        menu_set_error_msg(dictionary_get_string(DICT_LOST_CONNECTION_WITH_SERVER));
        return;
    }

/* Enter power save. Not used */
#if 0
    if (ctx.go_to_power_save_timeout < xTaskGetTickCount())
    {
        _enter_power_save();
        change_state(STATE_POWER_SAVE);
        return;
    }
#endif

    switch(ctx.substate)
    {
        case SUBSTATE_MAIN:
            _substate_main();
            break;

        case SUBSTATE_WATER_ADD:
            _substate_add_water();
            break;

        case SUBSTATE_APPLY_WATER:
            _substate_apply_water();
            break;

        case SUBSTATE_WAIT_WATER_ADD:
            _substate_wait_water_add();
            break;

        default:
            ctx.substate = SUBSTATE_MAIN;
            _substate_main();
            break;
    }

    backendEnterMenuStart();
}

static void menu_start_power_save(void)
{
    if (!backendIsConnected())
    {
        menu_set_error_msg(dictionary_get_string(DICT_LOST_CONNECTION_WITH_SERVER));
        return;
    }

    menuPrintfInfo(dictionary_get_string(DICT_POWER_SAVE));
    backendEnterMenuStart();
}

static void menu_start_error(void)
{
    static uint32_t blink_counter;
    static bool blink_state;
    bool motor_led_blink = false;
    bool servo_led_blink = false;

    MOTOR_LED_SET_GREEN(0);
    SERVO_VIBRO_LED_SET_GREEN(0);

    if (!backendIsConnected())
    {
        menu_set_error_msg(dictionary_get_string(DICT_LOST_CONNECTION_WITH_SERVER));
        return;
    }

    switch (ctx.error_dev)
    {

    default:
        menuPrintfInfo(dictionary_get_string(DICT_UNKNOWN_ERROR));
        motor_led_blink = true;
        servo_led_blink = true;
        break;
    }

    if ((blink_counter++) % 2 == 0)
    {
        MOTOR_LED_SET_RED(motor_led_blink ? blink_state : 0);
        SERVO_VIBRO_LED_SET_RED(servo_led_blink ? blink_state : 0);
        blink_state = blink_state ? false : true;
    }
}

static void menu_start_info(void)
{
    debug_function_name(__func__);
    osDelay(2500);
    change_state(ctx.last_state);
}


static void menu_start_stop(void)
{
}

static void menu_start_error_check(void)
{
    if (ctx.error_flag)
    {
        menuPrintfInfo(ctx.error_msg);
        ctx.error_flag = false;
    }
    else
    {
        change_state(STATE_INIT);
        osDelay(700);
    }
}

static void menu_reconnect(void)
{
    backendExitMenuStart();

    wifiDrvGetAPName(ctx.ap_name);
    if (strlen(ctx.ap_name) > 5)
    {
        wifiDrvConnect();
        change_state(STATE_WAIT_CONNECT);
    }
}

static void _show_wait_connection(void)
{
    oled_clearScreen();
    sprintf(ctx.buff, dictionary_get_string(DICT_WAIT_CONNECTION_S_S_S), xTaskGetTickCount() % 400 > 100 ? "." : " ",
        xTaskGetTickCount() % 400 > 200 ? "." : " ", xTaskGetTickCount() % 400 > 300 ? "." : " ");
    oled_printFixed(2, 2 * LINE_HEIGHT, ctx.buff, OLED_FONT_SIZE_11);
    oled_update();
}

static void menu_wait_connect(void)
{
    /* Wait to connect wifi */
    ctx.timeout_con = MS2ST(10000) + xTaskGetTickCount();
    ctx.exit_wait_flag = false;
    do
    {
        if ((ctx.timeout_con < xTaskGetTickCount()) || ctx.exit_wait_flag)
        {
            menu_set_error_msg(dictionary_get_string(DICT_TIMEOUT_CONNECT));
            return;
        }

        _show_wait_connection();
        osDelay(50);
    } while (wifiDrvTryingConnect());

    ctx.timeout_con = MS2ST(10000) + xTaskGetTickCount();
    do
    {
        if ((ctx.timeout_con < xTaskGetTickCount()) || ctx.exit_wait_flag)
        {
            menu_set_error_msg(dictionary_get_string(DICT_TIMEOUT_SERVER));
            return;
        }

        _show_wait_connection();
        osDelay(50);
    } while (!cmdClientIsConnected());

    oled_clearScreen();
    menuPrintfInfo(dictionary_get_string(DICT_CONNECTED_TRY_READ_DATA));
    change_state(STATE_CHECK_WIFI);
}

static bool menu_process(void *arg)
{
    menu_token_t *menu = arg;

    if (menu == NULL)
    {
        NULL_ERROR_MSG();
        return false;
    }

    switch (ctx.state)
    {
    case STATE_INIT:
        menu_start_init();
        break;

    case STATE_CHECK_WIFI:
        menu_check_connection();
        break;

    case STATE_IDLE:
        menu_start_idle();
        break;

    case STATE_START:
        menu_start_start();
        break;

    case STATE_READY:
        menu_start_ready();
        break;

    case STATE_POWER_SAVE:
        menu_start_power_save();
        break;

    case STATE_ERROR:
        menu_start_error();
        break;

    case STATE_INFO:
        menu_start_info();
        break;

    case STATE_STOP:
        menu_start_stop();
        break;

    case STATE_ERROR_CHECK:
        menu_start_error_check();
        break;

    case STATE_RECONNECT:
        menu_reconnect();
        break;

    case STATE_WAIT_CONNECT:
        menu_wait_connect();
        break;

    default:
        change_state(STATE_STOP);
        break;
    }

    if (backendIsEmergencyDisable() || ctx.state == STATE_ERROR || !backendIsConnected())
    {
        MOTOR_LED_SET_GREEN(0);
        SERVO_VIBRO_LED_SET_GREEN(0);
    }
    else
    {
        MOTOR_LED_SET_GREEN(ctx.data.valve[0].state);
        SERVO_VIBRO_LED_SET_GREEN(ctx.data.valve[1].state);
        MOTOR_LED_SET_RED(0);
        SERVO_VIBRO_LED_SET_RED(0);
    }

    return true;
}

void menuStartReset(void)
{
    for (int i = 0; i < CFG_VALVE_CNT; i++)
    {
        ctx.data.valve[i].state = false;
    }
}

void menuInitStartMenu(menu_token_t *menu)
{
    memset(&ctx, 0, sizeof(ctx));
    menu->menu_cb.enter = menu_enter_cb;
    menu->menu_cb.button_init_cb = menu_button_init_cb;
    menu->menu_cb.exit = menu_exit_cb;
    menu->menu_cb.process = menu_process;
}

void menuStartSetError(error_type_t error)
{
    LOG(PRINT_DEBUG, "%s %d", __func__, error);
    ctx.error_dev = error;
    change_state(STATE_ERROR);
    menuStartReset();
}

void menuStartResetError(void)
{
    LOG(PRINT_DEBUG, "%s", __func__);
    if (ctx.state == STATE_ERROR)
    {
        ctx.error_dev = ERROR_TOP;
        change_state(STATE_READY);
    }
}

struct menu_data *menuStartGetData(void)
{
    return &ctx.data;
}