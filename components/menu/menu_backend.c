#include <stdbool.h>

#include "app_config.h"
#include "but.h"
#include "cmd_client.h"
#include "dictionary.h"
#include "freertos/semphr.h"
#include "http_parameters_client.h"
#include "menu_drv.h"
#include "parameters.h"
#include "ssdFigure.h"
#include "start_menu.h"
#include "stdarg.h"
#include "stdint.h"
#include "wifidrv.h"

#define MODULE_NAME "[M BACK] "
#define DEBUG_LVL   PRINT_INFO

#if CONFIG_DEBUG_MENU_BACKEND
#define LOG( _lvl, ... ) \
  debug_printf( DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__ )
#else
#define LOG( PRINT_INFO, ... )
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
  char* error_msg;
  uint32_t get_data_cnt;

  bool enable_water_req;
  bool on_off_water;

  bool start_menu_is_active;
  bool menu_param_is_active;
  bool emergency_msg_sended;
  bool emergency_exit_msg_sended;
  bool emergency_req;

  bool send_all_data;
  struct menu_data sended_data;
} start_menu_context_t;

static start_menu_context_t ctx;

static char* state_name[] =
  {
    [STATE_INIT] = "STATE_INIT",
    [STATE_IDLE] = "STATE_IDLE",
    [STATE_START] = "STATE_START",
    [STATE_MENU_PARAMETERS] = "STATE_MENU_PARAMETERS",
    [STATE_ERROR_CHECK] = "STATE_ERROR_CHECK",
    [STATE_EMERGENCY_DISABLE] = "STATE_EMERGENCY_DISABLE",
    [STATE_EMERGENCY_DISABLE_EXIT] = "STATE_EMERGENCY_DISABLE_EXIT" };

static void change_state( state_backend_t new_state )
{
  if ( ctx.state < STATE_TOP )
  {
    if ( ctx.state != new_state )
    {
      LOG( PRINT_INFO, "Backend menu %s", state_name[new_state] );
      ctx.state = new_state;
    }
  }
}

static void _enter_emergency( void )
{
  if ( ctx.state != STATE_EMERGENCY_DISABLE )
  {
    LOG( PRINT_INFO, "%s %s", __func__, state_name[ctx.state] );
    change_state( STATE_EMERGENCY_DISABLE );
    ctx.emergency_msg_sended = false;
    ctx.emergency_exit_msg_sended = false;
    menuDrvEnterEmergencyDisable();
  }
}

static void _send_emergency_msg( void )
{
  if ( ctx.emergency_msg_sended )
  {
    return;
  }

  bool ret =
    ( HTTPParamClient_SetU32Value( PARAM_EMERGENCY_DISABLE, 1, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_1_STATE, 0, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_2_STATE, 0, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_3_STATE, 0, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_4_STATE, 0, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_5_STATE, 0, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_6_STATE, 0, 2000 ) == ERROR_CODE_OK )
    && ( HTTPParamClient_SetU32Value( PARAM_VALVE_7_STATE, 0, 2000 ) == ERROR_CODE_OK );

  LOG( PRINT_INFO, "%s %d", __func__, ret );
  if ( ret )
  {
    ctx.emergency_msg_sended = true;
    menuStartReset();
  }
}

static void _check_emergency_disable( void )
{
  if ( ctx.emergency_req )
  {
    _enter_emergency();
  }
}

static void backend_init_state( void )
{
  change_state( STATE_IDLE );
}

static void backend_idle( void )
{
  if ( ctx.menu_param_is_active )
  {
    change_state( STATE_MENU_PARAMETERS );
    return;
  }

  if ( ctx.start_menu_is_active )
  {
    ctx.send_all_data = true;
    change_state( STATE_START );
    return;
  }

  osDelay( 50 );
}

static bool _check_error( void )
{
  HTTPParamClient_GetU32Value( PARAM_MACHINE_ERRORS, NULL, 2000 );
  HTTPParamClient_GetU32Value( PARAM_WATER_FLOW_STATE, NULL, 2000 );
  // uint32_t errors = parameters_getValue( PARAM_MACHINE_ERRORS );

  // if ( errors > 0 )
  // {
  //   for ( uint8_t i = 0; i < ERROR_TOP; i++ )
  //   {
  //     if ( errors & ( 1 << i ) )
  //     {
  //       menuStartSetError( i );
  //     }
  //   }

  //   return true;
  // }

  return false;
}

static void backend_start( void )
{
  if ( ctx.get_data_cnt % 20 == 0 )
  {
    bool errors = _check_error() > 0;
    if ( errors )
    {
      LOG( PRINT_INFO, "Error detected on machine" );
    }
    else
    {
      menuStartResetError();
      LOG( PRINT_DEBUG, "No error" );
    }
    HTTPParamClient_GetU32Value( PARAM_VOLTAGE_ACCUM, NULL, 2000 );
    HTTPParamClient_GetU32Value( PARAM_LOW_LEVEL_SILOS, NULL, 2000 );
    HTTPParamClient_GetU32Value( PARAM_SILOS_LEVEL, NULL, 2000 );
    HTTPParamClient_GetU32Value( PARAM_SILOS_SENSOR_IS_CONNECTED, NULL, 2000 );
    HTTPParamClient_GetStrValue( PARAM_STR_CONTROLLER_SN, NULL, 0, 2000 );
  }

  ctx.get_data_cnt++;

  if ( ctx.menu_param_is_active )
  {
    change_state( STATE_MENU_PARAMETERS );
    return;
  }

  if ( !ctx.start_menu_is_active )
  {
    change_state( STATE_IDLE );
    return;
  }

  struct menu_data* data = menuStartGetData();

  if ( memcmp( &ctx.sended_data, data, sizeof( ctx.sended_data ) ) != 0 )
  {
    ctx.send_all_data = true;
  }

  if ( ctx.send_all_data )
  {
    uint8_t pass_counter = 0;

    for ( int i = 0; i < CFG_VALVE_CNT; i++ )
    {
      if ( HTTPParamClient_SetU32Value( PARAM_VALVE_1_STATE + i, data->valve[i].state, 1000 ) == ERROR_CODE_OK )
      {
        pass_counter++;
      }
    }

    if ( pass_counter == CFG_VALVE_CNT && ( HTTPParamClient_SetU32Value( PARAM_WATER_VOL_ADD, data->water_volume_l, 1000 ) == ERROR_CODE_OK ) )
    {
      ctx.send_all_data = false;
      for ( int i = 0; i < CFG_VALVE_CNT; i++ )
      {
        ctx.sended_data.valve[i].state = data->valve[i].state;
      }
      ctx.sended_data.water_volume_l = data->water_volume_l;
    }

    HTTPParamClient_SetU32Value( PARAM_PULSES_PER_LITER, parameters_getValue( PARAM_PULSES_PER_LITER ), 1000 );
    HTTPParamClient_SetU32Value( PARAM_PWM_VALVE, parameters_getValue( PARAM_PWM_VALVE ), 1000 );
  }

  if ( ctx.enable_water_req )
  {
    if ( HTTPParamClient_SetU32Value( PARAM_ADD_WATER, ctx.on_off_water, 1000 ) == ERROR_CODE_OK )
    {
      ctx.enable_water_req = false;
    }
  }

  if ( ctx.on_off_water && !ctx.enable_water_req )
  {
    if ( HTTPParamClient_GetU32Value( PARAM_ADD_WATER, NULL, 2000 ) == ERROR_CODE_OK )
    {
      ctx.on_off_water = parameters_getValue( PARAM_ADD_WATER );
    }

    HTTPParamClient_GetU32Value( PARAM_WATER_VOL_READ, NULL, 1000 );
  }

  osDelay( 10 );
}

static void backend_menu_parameters( void )
{
  if ( !ctx.menu_param_is_active )
  {
    change_state( STATE_IDLE );
    return;
  }

  HTTPParamClient_GetU32Value( PARAM_VOLTAGE_ACCUM, NULL, 2000 );
  osDelay( 50 );
}

static const char* _get_msg( menuDrvMsg_t msg )
{
  switch ( msg )
  {
    case MENU_DRV_MSG_WAIT_TO_INIT:
      return dictionary_get_string( DICT_WAIT_TO_INIT );

    case MENU_DRV_MSG_IDLE_STATE:
      return dictionary_get_string( DICT_MENU_IDLE_STATE );

    case MENU_DRV_MSG_MENU_STOP:
      return dictionary_get_string( DICT_MENU_STOP );

    case MENU_DRV_MSG_POWER_OFF:
      return dictionary_get_string( DICT_POWER_OFF );

    default:
      return NULL;
  }
}

static void backend_error_check( void )
{
  change_state( STATE_IDLE );
}

static void backend_emergency_disable_state( void )
{
  _send_emergency_msg();
  if ( !ctx.emergency_req )
  {
    LOG( PRINT_INFO, "%s exit", __func__ );
    menuDrvExitEmergencyDisable();
    change_state( STATE_EMERGENCY_DISABLE_EXIT );
  }

  osDelay( 50 );
}

static void backend_emergency_disable_exit( void )
{
  if ( !ctx.emergency_exit_msg_sended )
  {
    error_code_t ret = HTTPParamClient_SetU32Value( PARAM_EMERGENCY_DISABLE, 0, 2000 );
    LOG( PRINT_INFO, "%s %d", __func__, ret );
    if ( ret == ERROR_CODE_OK )
    {
      ctx.emergency_exit_msg_sended = true;
    }

    osDelay( 50 );
  }
  else
  {
    change_state( STATE_IDLE );
  }
}

void backendEnterparameterseters( void )
{
  ctx.menu_param_is_active = true;
}

void backendExitparameterseters( void )
{
  ctx.menu_param_is_active = false;
}

void backendEnterMenuStart( void )
{
  ctx.start_menu_is_active = true;
}

void backendExitMenuStart( void )
{
  ctx.start_menu_is_active = false;
}

void backendSetWater( bool on_off )
{
  parameters_setValue( PARAM_WATER_VOL_READ, 0 );
  ctx.enable_water_req = true;
  ctx.on_off_water = on_off;
}

void backendToggleEmergencyDisable( void )
{
  if ( ctx.emergency_req )
  {
    ctx.emergency_req = false;
  }
  else
  {
    if ( wifiDrvIsConnected() )
    {
      ctx.emergency_req = true;
    }
  }
}

static void menu_task( void* arg )
{
  while ( 1 )
  {
    _check_emergency_disable();

    switch ( ctx.state )
    {
      case STATE_INIT:
        backend_init_state();
        break;

      case STATE_IDLE:
        backend_idle();
        break;

      case STATE_START:
        backend_start();
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

void menuBackendInit( void )
{
  menuDrvSetGetMsgCb( _get_msg );
  menuDrvSetDrawBatteryCb( drawBattery );
  menuDrvSetDrawSignalCb( drawSignal );
  xTaskCreate( menu_task, "menu_back", 4096, NULL, 5, NULL );
}

bool backendIsConnected( void )
{
  if ( !wifiDrvIsConnected() )
  {
    LOG( PRINT_DEBUG, "START_MENU: WiFi not connected" );
    return false;
  }

  return true;
}

bool backendIsEmergencyDisable( void )
{
  return ctx.state == STATE_EMERGENCY_DISABLE;
}
