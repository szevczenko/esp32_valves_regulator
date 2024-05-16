#include <stdbool.h>

#include "cmd_server.h"
#include "error_valve.h"
#include "http_server.h"
#include "measure.h"
#include "parameters.h"
#include "pwm_drv.h"
#include "server_controller.h"
#include "water_flow_sensor.h"

#define MODULE_NAME "[Srvr Ctrl] "
#define DEBUG_LVL   PRINT_INFO

#if CONFIG_DEBUG_SERVER_CONTROLLER
#define LOG( _lvl, ... ) \
  debug_printf( DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__ )
#else
#define LOG( PRINT_INFO, ... )
#endif

#define SYSTEM_ON_PIN 15

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
  bool valve_on;    //Stan wysłany przez pilot
  bool state;    //Stan rzeczywisty
  int gpio;
  parameter_value_t menu_value;
};

typedef struct
{
  state_t state;

  bool system_on;
  bool emergency_disable;
  bool errors;

  bool water_on;
  uint32_t water_volume_l;
  uint32_t water_read_cl;    // value in centi liter: 1 l = 100 cl
  struct valve_data valves[CFG_VALVE_CNT];

  bool working_state_req;

  water_flow_sensor_t water_flow_sensor;
  pwm_drv_t valve_pwm;
} server_controller_ctx;

static server_controller_ctx ctx =
  {
    .valves = {
               { .gpio = 13,
        .menu_value = PARAM_VALVE_1_STATE },
               { .gpio = 14,
        .menu_value = PARAM_VALVE_2_STATE },
               { .gpio = 26,
        .menu_value = PARAM_VALVE_3_STATE },
               { .gpio = 25,
        .menu_value = PARAM_VALVE_4_STATE },
               { .gpio = 32,
        .menu_value = PARAM_VALVE_5_STATE },
               {
        .gpio = 18,
        .menu_value = PARAM_VALVE_6_STATE,
      },
               {
        .gpio = 19,
        .menu_value = PARAM_VALVE_7_STATE,
      } }
};

static char* state_name[] =
  {
    [STATE_INIT] = "STATE_INIT",
    [STATE_IDLE] = "STATE_IDLE",
    [STATE_WORKING] = "STATE_WORKING",
    [STATE_EMERGENCY_DISABLE] = "STATE_EMERGENCY_DISABLE",
    [STATE_ERROR] = "STATE_ERROR" };

static void change_state( state_t state )
{
  if ( state >= STATE_LAST )
  {
    return;
  }

  if ( state != ctx.state )
  {
    LOG( PRINT_INFO, "Change state -> %s", state_name[state] );
    ctx.state = state;
  }
}

static void count_working_data( void )
{
}

static void _on_valve( struct valve_data* valve )
{
  printf( "[VALVE] %d ON\n\r", valve->gpio );
  PWMDrv_SetDuty( &ctx.valve_pwm, (float) parameters_getValue( PARAM_PWM_VALVE ) );
  gpio_set_level( valve->gpio, 1 );
  osDelay( 20 );
  PWMDrv_SetDuty( &ctx.valve_pwm, 100 );
  osDelay( 80 );
  PWMDrv_SetDuty( &ctx.valve_pwm, (float) parameters_getValue( PARAM_PWM_VALVE ) );
  valve->state = 1;
}

static void _off_valve( struct valve_data* valve )
{
  printf( "[VALVE] %d OFF\n\r", valve->gpio );
  gpio_set_level( valve->gpio, 0 );
  valve->state = 0;
}

static void set_working_data( void )
{
  if ( ctx.system_on )
  {
    gpio_set_level( SYSTEM_ON_PIN, 1 );
  }
  else
  {
    gpio_set_level( SYSTEM_ON_PIN, 0 );
  }

  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    if ( ctx.valves[i].valve_on != ctx.valves[i].state )
    {
      if ( ctx.valves[i].valve_on )
      {
        _on_valve( &ctx.valves[i] );
      }
      else
      {
        _off_valve( &ctx.valves[i] );
      }
    }
  }
}

static void water_flow_event_callback( water_flow_sensor_event_t event, uint32_t value )
{
  switch ( event )
  {
    case WATER_FLOW_SENSOR_EVENT_WATER_FLOW_BACK:
      parameters_setValue( PARAM_WATER_FLOW_STATE, 0 );
      break;

    case WATER_FLOW_SENSOR_EVENT_NO_WATER_SHORT_PERIOD:
      parameters_setValue( PARAM_WATER_FLOW_STATE, 1 );
      break;

    case WATER_FLOW_SENSOR_EVENT_NO_WATER_LONG_PERIOD:
      parameters_setValue( PARAM_WATER_FLOW_STATE, 2 );
      break;

    default:
      break;
  }
}

static void state_init( void )
{
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = ( 1ULL << SYSTEM_ON_PIN );
  gpio_config( &io_conf );

  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    io_conf.pin_bit_mask = BIT64( ctx.valves[i].gpio );
    gpio_config( &io_conf );
  }

  WaterFlowSensor_Init( &ctx.water_flow_sensor, "valve1", parameters_getValue( PARAM_PULSES_PER_LITER ), water_flow_event_callback, 34 );
  PWMDrv_Init( &ctx.valve_pwm, "valve_pwm", PWM_DRV_DUTY_MODE_LOW, 1000, 0, CFG_VALVE_CURRENT_REGULATION_PIN );

  parameters_setValue( PARAM_VALVE_1_STATE, 0 );
  parameters_setValue( PARAM_VALVE_2_STATE, 0 );
  parameters_setValue( PARAM_VALVE_3_STATE, 0 );
  parameters_setValue( PARAM_VALVE_4_STATE, 0 );
  parameters_setValue( PARAM_VALVE_5_STATE, 0 );
  parameters_setValue( PARAM_VALVE_6_STATE, 0 );
  parameters_setValue( PARAM_VALVE_7_STATE, 0 );
  parameters_setValue( PARAM_ADD_WATER, 0 );
  parameters_setValue( PARAM_WATER_VOL_ADD, 0 );
  parameters_setValue( PARAM_WATER_VOL_READ, 0 );
  parameters_setValue( PARAM_MACHINE_ERRORS, 0 );
  parameters_setValue( PARAM_START_SYSTEM, 0 );
  parameters_setValue( PARAM_WATER_FLOW_STATE, 0 );

  change_state( STATE_IDLE );
}

static void state_idle( void )
{
  ctx.working_state_req = (bool) parameters_getValue( PARAM_START_SYSTEM );
  ctx.emergency_disable = (bool) parameters_getValue( PARAM_EMERGENCY_DISABLE );

  if ( ctx.emergency_disable )
  {
    change_state( STATE_EMERGENCY_DISABLE );
    return;
  }

  if ( ctx.working_state_req && HTTPServer_IsClientConnected() )
  {
    measure_meas_calibration_value();
    count_working_data();
    set_working_data();
    osDelay( 1000 );
    change_state( STATE_WORKING );
    return;
  }

  osDelay( 100 );
}

static void state_working( void )
{
  ctx.system_on = (bool) parameters_getValue( PARAM_START_SYSTEM );

  ctx.working_state_req = (bool) parameters_getValue( PARAM_START_SYSTEM );
  ctx.emergency_disable = (bool) parameters_getValue( PARAM_EMERGENCY_DISABLE );

  if ( ctx.emergency_disable )
  {
    change_state( STATE_EMERGENCY_DISABLE );
    return;
  }

  if ( !ctx.working_state_req || !HTTPServer_IsClientConnected() )
  {
    change_state( STATE_IDLE );
    return;
  }

  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    ctx.valves[i].valve_on = parameters_getValue( ctx.valves[i].menu_value );
  }

  ctx.water_volume_l = parameters_getValue( PARAM_WATER_VOL_ADD );

  if ( !ctx.water_on && parameters_getValue( PARAM_ADD_WATER ) )
  {
    WaterFlowSensor_StartMeasure( &ctx.water_flow_sensor );
    ctx.water_read_cl = 0;
  }

  ctx.water_on = parameters_getValue( PARAM_ADD_WATER );

  if ( ctx.water_read_cl >= WATER_FLOW_CONVERT_L_TO_CL( ctx.water_volume_l ) )
  {
    WaterFlowSensor_StopMeasure( &ctx.water_flow_sensor );
    parameters_setValue( PARAM_ADD_WATER, 0 );
    ctx.water_on = false;
  }

  parameters_setValue( PARAM_VALVE_6_STATE, ctx.water_on );
  parameters_setValue( PARAM_WATER_VOL_READ, ctx.water_read_cl );

  ctx.water_read_cl = WaterFlowSensor_GetValue( &ctx.water_flow_sensor );
  WaterFlowSensor_SetPulsesPerLiter( &ctx.water_flow_sensor, parameters_getValue( PARAM_PULSES_PER_LITER ) );

  osDelay( 100 );
}

static void state_emergency_disable( void )
{
  // Tą linijke usunąć jeżeli niepotrzebne wyłączenie przekaźnika w trybie STOP
  ctx.system_on = 0;
  ctx.emergency_disable = (bool) parameters_getValue( PARAM_EMERGENCY_DISABLE );

  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    ctx.valves[i].valve_on = false;
  }

  ctx.water_on = false;

  if ( !ctx.emergency_disable )
  {
    change_state( STATE_IDLE );
    return;
  }

  osDelay( 100 );
}

static void state_error( void )
{
  for ( int i = 0; i < CFG_VALVE_CNT; i++ )
  {
    ctx.valves[i].valve_on = false;
  }

  ctx.water_on = false;

  ctx.errors = (bool) parameters_getValue( PARAM_MACHINE_ERRORS );

  if ( !ctx.errors )
  {
    errorReset();
    change_state( STATE_IDLE );
    return;
  }

  osDelay( 100 );
}

static void _task( void* arg )
{
  parameters_setString( PARAM_STR_CONTROLLER_SN, DevConfig_GetSerialNumber() );
  while ( 1 )
  {
    switch ( ctx.state )
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
        change_state( STATE_IDLE );
        break;
    }

    count_working_data();
    set_working_data();
  }
}

bool srvrControllIsWorking( void )
{
  return ctx.state == STATE_WORKING;
}

bool srvrControllGetEmergencyDisable( void )
{
  return ctx.emergency_disable;
}

void srvrControllStart( void )
{
  xTaskCreate( _task, "srvrController", 4096, NULL, 10, NULL );
}

bool srvrcontrollerSetError( uint16_t error_reason )
{
  if ( ctx.state == STATE_WORKING )
  {
    change_state( STATE_ERROR );
    uint16_t error = ( 1 << error_reason );
    parameters_setValue( PARAM_MACHINE_ERRORS, error );
    return true;
  }

  return false;
}

bool srvrControllerErrorReset( void )
{
  if ( ctx.state == STATE_ERROR )
  {
    errorReset();
    change_state( STATE_IDLE );
    return true;
  }

  return false;
}
