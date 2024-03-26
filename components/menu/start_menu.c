#include "start_menu.h"

#include "app_config.h"
#include "battery.h"
#include "buzzer.h"
#include "cmd_client.h"
#include "fast_add.h"
#include "freertos/timers.h"
#include "menu_backend.h"
#include "menu_default.h"
#include "menu_drv.h"
#include "oled.h"
#include "ssd1306.h"
#include "ssdFigure.h"
#include "string.h"
#include "wifidrv.h"

#define MODULE_NAME "[START] "
#define DEBUG_LVL   PRINT_INFO

#if CONFIG_DEBUG_MENU_BACKEND
#define LOG( _lvl, ... ) \
  debug_printf( DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__ )
#else
#define LOG( PRINT_INFO, ... )
#endif

#define DEVICE_LIST_SIZE         16
#define CHANGE_MENU_TIMEOUT_MS   1500
#define POWER_SAVE_TIMEOUT_MS    30 * 1000
#define CHANGE_VALUE_DISP_OFFSET 40
#define PARAM_START_OFFSET       42

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
  const char* error_msg;
  const char* info_msg;
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
} context_t;

static context_t ctx;

loadBar_t motor_bar =
  {
    .x = 40,
    .y = 10,
    .width = 80,
    .height = 10,
};

loadBar_t servo_bar =
  {
    .x = 40,
    .y = 40,
    .width = 80,
    .height = 10,
};

static char* state_name[] =
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
    [STATE_WAIT_CONNECT] = "STATE_WAIT_CONNECT" };

static void change_state( state_start_menu_t new_state )
{
  if ( ctx.state < STATE_TOP )
  {
    if ( ctx.state != new_state )
    {
      LOG( PRINT_INFO, "Start menu %s", state_name[new_state] );
    }

    ctx.state = new_state;
  }
  else
  {
    LOG( PRINT_INFO, "change state %ld", new_state );
  }
}

static void _reset_error( void )
{
  if ( parameters_getValue( PARAM_MACHINE_ERRORS ) )
  {
    cmdClientSetValueWithoutResp( PARAM_MACHINE_ERRORS, 0 );
  }
}

static bool _is_working_state( void )
{
  if ( ctx.state == STATE_READY )
  {
    return true;
  }

  return false;
}

static bool _is_power_save( void )
{
  if ( ctx.state == STATE_POWER_SAVE )
  {
    return true;
  }

  return false;
}

// static void _enter_power_save(void)
// {
//  wifiDrvPowerSave(true);
// }

static void _exit_power_save( void )
{
  wifiDrvPowerSave( false );
}

static void _reset_power_save_timer( void )
{
  ctx.go_to_power_save_timeout = MS2ST( POWER_SAVE_TIMEOUT_MS ) + xTaskGetTickCount();
}

static bool default_button_process( void* arg )
{
  menu_token_t* menu = arg;

  if ( menu == NULL )
  {
    NULL_ERROR_MSG();
    return false;
  }

  _reset_error();
  _reset_power_save_timer();

  if ( _is_power_save() )
  {
    _exit_power_save();
    change_state( STATE_READY );
  }

  if ( !_is_working_state() )
  {
    return false;
  }

  return true;
}

static void fast_add_callback( uint32_t value )
{
}

static void menu_button_up_callback( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  if ( ctx.substate == SUBSTATE_WATER_ADD )
  {
    if ( ctx.data.water_volume_l < parameters_getValue( PARAM_TANK_SIZE ) )
    {
      ctx.data.water_volume_l += 10;
    }
  }
}

static void menu_button_up_time_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  if ( ctx.substate == SUBSTATE_WATER_ADD )
  {
    fastProcessStart( &ctx.data.water_volume_l, parameters_getValue( PARAM_TANK_SIZE ), 1, FP_PLUS_10, fast_add_callback );
  }
  else
  {
    menuDrv_EnterToParameters();
  }
}

static void menu_button_up_down_pull_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  fastProcessStop( &ctx.data.water_volume_l );
}

static void menu_button_down_callback( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  if ( ctx.substate == SUBSTATE_WATER_ADD )
  {
    if ( ctx.data.water_volume_l >= 10 )
    {
      ctx.data.water_volume_l -= 10;
    }
  }
}

static void menu_button_down_time_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  if ( ctx.substate == SUBSTATE_WATER_ADD )
  {
    fastProcessStart( &ctx.data.water_volume_l, parameters_getValue( PARAM_TANK_SIZE ), 0, FP_MINUS_10, fast_add_callback );
  }
  else
  {
    menuDrv_EnterToParameters();
  }
}

static void menu_button_exit_callback( void* arg )
{
  menu_token_t* menu = arg;

  if ( menu == NULL )
  {
    NULL_ERROR_MSG();
    return;
  }
  _reset_error();
  if ( ctx.substate == SUBSTATE_MAIN )
  {
    menuDrv_Exit( menu );
  }
  else
  {
    backendSetWater( false );
    ctx.substate = SUBSTATE_MAIN;
  }

  ctx.exit_wait_flag = true;
}

static void menu_button_valve_5_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  ctx.data.valve[4].state = ctx.data.valve[4].state ? false : true;
}

static void menu_button_add_water_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  if ( ctx.substate == SUBSTATE_WATER_ADD )
  {
    ctx.substate = SUBSTATE_APPLY_WATER;
  }
  else if ( ctx.substate == SUBSTATE_APPLY_WATER )
  {
    backendSetWater( true );
    parameters_setValue( PARAM_ADD_WATER, 1 );
    ctx.substate = SUBSTATE_WAIT_WATER_ADD;
  }
}

static void menu_button_add_timer_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  if ( ctx.substate == SUBSTATE_MAIN )
  {
    ctx.substate = SUBSTATE_WATER_ADD;
  }
}

static void menu_button_valve_2_push_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  ctx.data.valve[1].state = ctx.data.valve[1].state ? false : true;
}

static void menu_button_valve_2_time_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }
}

static void menu_button_valve_1_push_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  ctx.data.valve[0].state = ctx.data.valve[0].state ? false : true;
}

static void menu_button_valve_1_time_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }
}

static void menu_button_valve_1_2_pull_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  menuDrvSaveParameters();
}

/*-------------DOWN BUTTONS------------*/

static void menu_button_valve_4_push_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  ctx.data.valve[3].state = ctx.data.valve[3].state ? false : true;
}

static void menu_button_valve_4_time_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }
}

static void menu_button_valve_3_push_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  ctx.data.valve[2].state = ctx.data.valve[2].state ? false : true;
}

static void menu_button_valve_3_time_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }
}

static void menu_button_valve_3_4_pull_cb( void* arg )
{
  if ( !default_button_process( arg ) )
  {
    return;
  }

  menuDrvSaveParameters();
}

static void menu_button_on_off( void* arg )
{
  menu_token_t* menu = arg;

  if ( menu == NULL )
  {
    NULL_ERROR_MSG();
    return;
  }

  _reset_error();
}

static bool menu_button_init_cb( void* arg )
{
  menu_token_t* menu = arg;

  if ( menu == NULL )
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

static bool menu_enter_cb( void* arg )
{
  menu_token_t* menu = arg;

  if ( menu == NULL )
  {
    NULL_ERROR_MSG();
    return false;
  }

  _exit_power_save();
  _reset_power_save_timer();

  if ( !backendIsConnected() )
  {
    change_state( STATE_INIT );
  }

  cmdClientSetValueWithoutResp( PARAM_START_SYSTEM, 1 );

  backendEnterMenuStart();

  ctx.error_flag = 0;
  return true;
}

static bool menu_exit_cb( void* arg )
{
  backendExitMenuStart();

  menu_token_t* menu = arg;

  if ( menu == NULL )
  {
    NULL_ERROR_MSG();
    return false;
  }

  MOTOR_LED_SET_GREEN( 0 );
  SERVO_VIBRO_LED_SET_GREEN( 0 );
  return true;
}

static void menu_set_error_msg( const char* msg )
{
  ctx.error_msg = msg;
  ctx.error_flag = 1;
  change_state( STATE_ERROR_CHECK );
}

static void start_menu_init( void )
{
  oled_printFixed( 2, 2 * LINE_HEIGHT, dictionary_get_string( DICT_CHECK_CONNECTION ), OLED_FONT_SIZE_11 );
  change_state( STATE_CHECK_WIFI );
}

static void menu_check_connection( void )
{
  if ( !backendIsConnected() )
  {
    change_state( STATE_RECONNECT );
    return;
  }

  bool ret = false;

  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    ctx.data.valve[i].state = parameters_getValue( PARAM_VALVE_1_STATE + i );
  }

  for ( uint8_t i = 0; i < 3; i++ )
  {
    LOG( PRINT_INFO, "START_MENU: cmdClientGetAllValue try %ld", i );
    osDelay( 250 );

    if ( cmdClientSetValue( PARAM_EMERGENCY_DISABLE, 0, 1000 ) == ERROR_CODE_OK )
    {
      break;
    }
  }

  if ( ret != true )
  {
    LOG( PRINT_INFO, "%s: error get parameters", __func__ );
    change_state( STATE_IDLE );
  }

  change_state( STATE_IDLE );
}

static void start_menu_idle( void )
{
  if ( backendIsConnected() )
  {
    cmdClientSetValueWithoutResp( PARAM_START_SYSTEM, 1 );
    change_state( STATE_START );
  }
  else
  {
    menuPrintfInfo( "Target not connected.\nGo to DEVICES\nfor connect" );
  }
}

static void start_menu_start( void )
{
  if ( !backendIsConnected() )
  {
    return;
  }

  change_state( STATE_READY );
}

static void _substate_main( void )
{
  static char buff_on[64];
  static char buff_off[64];

  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    sprintf( buff_on, "Valve on:  %s %s %s %s %s",
             ctx.data.valve[0].state ? "1" : "_",
             ctx.data.valve[1].state ? "2" : "_",
             ctx.data.valve[2].state ? "3" : "_",
             ctx.data.valve[3].state ? "4" : "_",
             ctx.data.valve[4].state ? "5" : "_" );

    sprintf( buff_off, "Valve off: %s %s %s %s %s",
             !ctx.data.valve[0].state ? "1" : "_",
             !ctx.data.valve[1].state ? "2" : "_",
             !ctx.data.valve[2].state ? "3" : "_",
             !ctx.data.valve[3].state ? "4" : "_",
             !ctx.data.valve[4].state ? "5" : "_" );
  }

  oled_printFixed( 2, MENU_HEIGHT, buff_on, OLED_FONT_SIZE_11 );
  oled_printFixed( 2, MENU_HEIGHT * 2, buff_off, OLED_FONT_SIZE_11 );
}

static void _get_liters_coordinate_and_size( uint32_t value, uint8_t* x, uint8_t* y, enum oledFontSize* font )
{
  *x = 40;
  *y = 25;
  *font = OLED_FONT_SIZE_26;
  if ( value < 10 )
  {
    *x = 40;
  }
  else if ( value < 100 )
  {
    *x = 30;
  }
  else if ( value < 1000 )
  {
    *x = 20;
  }
  else if ( value < 10000 )
  {
    *x = 17;
  }
  else
  {
    *font = OLED_FONT_SIZE_16;
    *x = 20;
    *y = 30;
  }
}

static void _substate_add_water( void )
{
  static char buff_water[64];
  ssdFigure_DrawArrow( 2, 10, true );
  ssdFigure_DrawArrow( 2, 38, false );
  ssdFigure_DrawValve( 105, 15, false );
  ssdFigure_DrawTank( 90, 40, ctx.data.water_volume_l * 100 / parameters_getValue( PARAM_TANK_SIZE ) );
  oled_printFixed( 20, 2, "ADD WATER:", OLED_FONT_SIZE_11 );
  sprintf( buff_water, "%ld [l]", ctx.data.water_volume_l );

  uint8_t x_liters;
  uint8_t y_liters;
  enum oledFontSize font_liters;
  _get_liters_coordinate_and_size( ctx.data.water_volume_l, &x_liters, &y_liters, &font_liters );
  oled_printFixed( x_liters, y_liters, buff_water, font_liters );
}

static void _substate_apply_water( void )
{
  oled_printFixed( 2, 2, "Add water?", OLED_FONT_SIZE_11 );
  oled_printFixed( 11, 14, "Yes", OLED_FONT_SIZE_11 );
  oled_printFixed( 98, 14, "No", OLED_FONT_SIZE_11 );
  ssdFigure_DrawAcceptButton( 2, 26 );
  ssdFigure_DrawDeclineButton( 88, 26 );
}

static void _substate_wait_water_add( void )
{
  static char buff_water[64];
  uint32_t water_added = parameters_getValue( PARAM_WATER_VOL_READ );
  uint16_t water_flow_state = parameters_getValue( PARAM_WATER_FLOW_STATE );

  if ( water_flow_state == 0 )
  {
    oled_printFixed( 2, 2, "Water added:", OLED_FONT_SIZE_11 );
    ssdFigure_DrawValveAnimation( 105, 15, ctx.animation_cnt );
  }
  else if ( water_flow_state == 1 )
  {
    oled_printFixed( 2, 2, "No water flow", OLED_FONT_SIZE_11 );
    ssdFigure_DrawValve( 105, 15, false );
  }
  else
  {
    oled_printFixed( 2, 2, "Error water flow", OLED_FONT_SIZE_11 );
    ssdFigure_DrawValve( 105, 15, true );
  }

  ssdFigure_DrawTank( 90, 40, water_added * 100 / parameters_getValue( PARAM_TANK_SIZE ) );
  sprintf( buff_water, "%ld [l]", water_added );

  uint8_t x_liters;
  uint8_t y_liters;
  enum oledFontSize font_liters;
  _get_liters_coordinate_and_size( water_added, &x_liters, &y_liters, &font_liters );
  oled_printFixed( x_liters, y_liters, buff_water, font_liters );

  if ( water_added > ctx.data.water_volume_l - 10 || !parameters_getValue( PARAM_ADD_WATER ) )
  {
    ctx.substate = SUBSTATE_MAIN;
    if ( parameters_getValue( PARAM_ADD_WATER ) )
    {
      backendSetWater( false );
    }
  }
  if ( ctx.animation_timeout < xTaskGetTickCount() )
  {
    ctx.animation_cnt++;
    ctx.animation_timeout = xTaskGetTickCount() + MS2ST( 200 );
  }
}

static void start_menu_ready( void )
{
  if ( !backendIsConnected() )
  {
    menu_set_error_msg( dictionary_get_string( DICT_LOST_CONNECTION_WITH_SERVER ) );
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

  switch ( ctx.substate )
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

static void start_menu_power_save( void )
{
  if ( !backendIsConnected() )
  {
    menu_set_error_msg( dictionary_get_string( DICT_LOST_CONNECTION_WITH_SERVER ) );
    return;
  }

  menuPrintfInfo( dictionary_get_string( DICT_POWER_SAVE ) );
  backendEnterMenuStart();
}

static void start_menu_error( void )
{
  static uint32_t blink_counter;
  static bool blink_state;
  bool motor_led_blink = false;
  bool servo_led_blink = false;

  MOTOR_LED_SET_GREEN( 0 );
  SERVO_VIBRO_LED_SET_GREEN( 0 );

  if ( !backendIsConnected() )
  {
    menu_set_error_msg( dictionary_get_string( DICT_LOST_CONNECTION_WITH_SERVER ) );
    return;
  }

  switch ( ctx.error_dev )
  {
    default:
      menuPrintfInfo( dictionary_get_string( DICT_UNKNOWN_ERROR ) );
      motor_led_blink = true;
      servo_led_blink = true;
      break;
  }

  if ( ( blink_counter++ ) % 2 == 0 )
  {
    MOTOR_LED_SET_RED( motor_led_blink ? blink_state : 0 );
    SERVO_VIBRO_LED_SET_RED( servo_led_blink ? blink_state : 0 );
    blink_state = blink_state ? false : true;
  }
}

static void start_menu_info( void )
{
  osDelay( 2500 );
  change_state( ctx.last_state );
}

static void start_menu_stop( void )
{
  backendSetWater( false );
}

static void start_menu_error_check( void )
{
  if ( ctx.error_flag )
  {
    menuPrintfInfo( ctx.error_msg );
    ctx.error_flag = false;
  }
  else
  {
    change_state( STATE_INIT );
    osDelay( 700 );
  }
}

static void menu_reconnect( void )
{
  backendExitMenuStart();

  wifiDrvGetAPName( ctx.ap_name );
  if ( strlen( ctx.ap_name ) > 5 )
  {
    wifiDrvConnect();
    change_state( STATE_WAIT_CONNECT );
  }
}

static void _show_wait_connection( void )
{
  oled_clearScreen();
  sprintf( ctx.buff, dictionary_get_string( DICT_WAIT_CONNECTION_S_S_S ), xTaskGetTickCount() % 400 > 100 ? "." : " ",
           xTaskGetTickCount() % 400 > 200 ? "." : " ", xTaskGetTickCount() % 400 > 300 ? "." : " " );
  oled_printFixed( 2, 2 * LINE_HEIGHT, ctx.buff, OLED_FONT_SIZE_11 );
  oled_update();
}

static void menu_wait_connect( void )
{
  /* Wait to connect wifi */
  ctx.timeout_con = MS2ST( 10000 ) + xTaskGetTickCount();
  ctx.exit_wait_flag = false;
  do
  {
    if ( ( ctx.timeout_con < xTaskGetTickCount() ) || ctx.exit_wait_flag )
    {
      menu_set_error_msg( dictionary_get_string( DICT_TIMEOUT_CONNECT ) );
      return;
    }

    _show_wait_connection();
    osDelay( 50 );
  } while ( wifiDrvTryingConnect() );

  ctx.timeout_con = MS2ST( 10000 ) + xTaskGetTickCount();
  do
  {
    if ( ( ctx.timeout_con < xTaskGetTickCount() ) || ctx.exit_wait_flag )
    {
      menu_set_error_msg( dictionary_get_string( DICT_TIMEOUT_SERVER ) );
      return;
    }

    _show_wait_connection();
    osDelay( 50 );
  } while ( !cmdClientIsConnected() );

  oled_clearScreen();
  menuPrintfInfo( dictionary_get_string( DICT_CONNECTED_TRY_READ_DATA ) );
  change_state( STATE_CHECK_WIFI );
}

static bool menu_process( void* arg )
{
  menu_token_t* menu = arg;

  if ( menu == NULL )
  {
    NULL_ERROR_MSG();
    return false;
  }

  switch ( ctx.state )
  {
    case STATE_INIT:
      start_menu_init();
      break;

    case STATE_CHECK_WIFI:
      menu_check_connection();
      break;

    case STATE_IDLE:
      start_menu_idle();
      break;

    case STATE_START:
      start_menu_start();
      break;

    case STATE_READY:
      start_menu_ready();
      break;

    case STATE_POWER_SAVE:
      start_menu_power_save();
      break;

    case STATE_ERROR:
      start_menu_error();
      break;

    case STATE_INFO:
      start_menu_info();
      break;

    case STATE_STOP:
      start_menu_stop();
      break;

    case STATE_ERROR_CHECK:
      start_menu_error_check();
      break;

    case STATE_RECONNECT:
      menu_reconnect();
      break;

    case STATE_WAIT_CONNECT:
      menu_wait_connect();
      break;

    default:
      change_state( STATE_STOP );
      break;
  }

  if ( backendIsEmergencyDisable() || ctx.state == STATE_ERROR || !backendIsConnected() )
  {
    MOTOR_LED_SET_GREEN( 0 );
    SERVO_VIBRO_LED_SET_GREEN( 0 );
  }
  else
  {
    MOTOR_LED_SET_GREEN( ctx.data.valve[0].state );
    SERVO_VIBRO_LED_SET_GREEN( ctx.data.valve[1].state );
    MOTOR_LED_SET_RED( 0 );
    SERVO_VIBRO_LED_SET_RED( 0 );
  }

  return true;
}

void menuStartReset( void )
{
  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    ctx.data.valve[i].state = false;
  }
}

void menuInitStartMenu( menu_token_t* menu )
{
  memset( &ctx, 0, sizeof( ctx ) );
  menu->menu_cb.enter = menu_enter_cb;
  menu->menu_cb.button_init_cb = menu_button_init_cb;
  menu->menu_cb.exit = menu_exit_cb;
  menu->menu_cb.process = menu_process;
}

void menuStartSetError( error_type_t error )
{
  LOG( PRINT_DEBUG, "%s %ld", __func__, error );
  ctx.error_dev = error;
  change_state( STATE_ERROR );
  menuStartReset();
}

void menuStartResetError( void )
{
  LOG( PRINT_DEBUG, "%s", __func__ );
  if ( ctx.state == STATE_ERROR )
  {
    ctx.error_dev = ERROR_TOP;
    change_state( STATE_READY );
  }
}

struct menu_data* menuStartGetData( void )
{
  return &ctx.data;
}