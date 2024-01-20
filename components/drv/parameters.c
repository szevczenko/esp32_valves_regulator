#include "parameters.h"

#include "menu.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "parse_cmd.h"

#define MODULE_NAME "[PARAM] "
#define DEBUG_LVL   PRINT_INFO

#if CONFIG_DEBUG_MENU_BACKEND
#define LOG( _lvl, ... ) \
  debug_printf( DEBUG_LVL, _lvl, MODULE_NAME __VA_ARGS__ )
#else
#define LOG( PRINT_INFO, ... )
#endif

#define STORAGE_NAMESPACE   "parameters"
#define PARAMETERS_TAB_SIZE PARAM_LAST_VALUE

static parameter_t parameters[] =
  {
    [PARAM_VALVE_1_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_1_STATE" },
    [PARAM_VALVE_2_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_2_STATE" },
    [PARAM_VALVE_3_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_3_STATE" },
    [PARAM_VALVE_4_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_4_STATE" },
    [PARAM_VALVE_5_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_5_STATE" },
    [PARAM_VALVE_6_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_6_STATE" },
    [PARAM_VALVE_7_STATE] = { .max_value = 1, .default_value = 0, .name = "PARAM_VALVE_7_STATE" },
    [PARAM_ADD_WATER] = { .max_value = 1, .default_value = 0, .name = "PARAM_ADD_WATER" },
    [PARAM_WATER_VOL_ADD] = { .max_value = 2500, .default_value = 100, .name = "PARAM_WATER_VOL_ADD" },
    [PARAM_WATER_VOL_READ] = { .max_value = 10000, .default_value = 0, .name = "PARAM_WATER_VOL_READ" },
    [PARAM_VOLTAGE_ACCUM] = { .max_value = 1, .default_value = 0, .name = "PARAM_VOLTAGE_ACCUM" },
    [PARAM_START_SYSTEM] = { .max_value = 1, .default_value = 0, .name = "PARAM_START_SYSTEM" },
    [PARAM_BOOTUP_SYSTEM] = { .max_value = 1, .default_value = 1, .name = "PARAM_BOOTUP_SYSTEM" },
    [PARAM_EMERGENCY_DISABLE] = { .max_value = 1, .default_value = 0, .name = "PARAM_EMERGENCY_DISABLE" },
    [PARAM_SILOS_LEVEL] = { .max_value = 100, .default_value = 0, .name = "PARAM_SILOS_LEVEL" },
    [PARAM_SILOS_HEIGHT] = { .max_value = 300, .default_value = 60, .name = "PARAM_SILOS_HEIGHT" },
    [PARAM_LOW_LEVEL_SILOS] = { .max_value = 1, .default_value = 0, .name = "PARAM_LOW_LEVEL_SILOS" },
    [PARAM_SILOS_SENSOR_IS_CONECTED] = { .max_value = 1, .default_value = 0, .name = "PARAM_SILOS_SENSOR_IS_CONECTED" },
    [PARAM_LANGUAGE] = { .max_value = 3, .default_value = 0, .name = "PARAM_LANGUAGE" },
    [PARAM_POWER_ON_MIN] = { .min_value = 5, .max_value = 100, .default_value = 30, .name = "PARAM_POWER_ON_MIN" },

    [PARAM_MACHINE_ERRORS] = { .max_value = 0xFFFF, .default_value = 0, .name = "PARAM_MACHINE_ERRORS" },

    [PARAM_BUZZER] = { .max_value = 1, .default_value = 1, .name = "PARAM_BUZZER" },
    [PARAM_BRIGHTNESS] = { .max_value = 10, .default_value = 10, .name = "PARAM_BRIGHTNESS" },
};

static uint32_t parameters_value[PARAM_LAST_VALUE];

static bool _read_parameters( void )
{
  nvs_handle my_handle;
  esp_err_t err;
  size_t required_size = 0;

  err = nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle );
  if ( err != ESP_OK )
  {
    return false;
  }

  err = nvs_get_blob( my_handle, "menu", NULL, &required_size );
  if ( ( err != ESP_OK ) && ( err != ESP_ERR_NVS_NOT_FOUND ) )
  {
    nvs_close( my_handle );
    return false;
  }

  if ( required_size == sizeof( parameters_value ) )
  {
    err = nvs_get_blob( my_handle, "menu", parameters_value, &required_size );
    nvs_close( my_handle );
    return true;
  }

  nvs_close( my_handle );
  return false;
}

static bool _check_values( void )
{
  for ( uint8_t i = 0; i < sizeof( parameters_value ) / sizeof( uint32_t ); i++ )
  {
    if ( parameters_value[i] > parameters_getMaxValue( i ) || parameters_value[i] < parameters_getMinValue( i ) )
    {
      return false;
    }
  }

  return true;
}

void parameters_debugPrint( void )
{
  for ( uint8_t i = 0; i < PARAMETERS_TAB_SIZE; i++ )
  {
    LOG( PRINT_DEBUG, "%s : %d\n", parameters[i].name, parameters_value[i] );
  }
}

void parameters_debugPrintValue( parameter_value_t val )
{
  if ( val >= PARAM_LAST_VALUE )
  {
    return;
  }

  LOG( PRINT_DEBUG, "Param: %s : %d", parameters[val].name, parameters_value[val] );
}

bool parameters_save( void )
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle );
  if ( err != ESP_OK )
  {
    LOG( PRINT_ERROR, "nvs_open error %d", err );
    nvs_close( my_handle );
    return false;
  }

  err = nvs_set_blob( my_handle, "menu", parameters_value, sizeof( parameters_value ) );

  if ( err != ESP_OK )
  {
    LOG( PRINT_ERROR, "nvs_set_blob error %d", err );
    nvs_close( my_handle );
    return false;
  }

  // Commit
  err = nvs_commit( my_handle );
  if ( err != ESP_OK )
  {
    LOG( PRINT_ERROR, "nvs_commit error %d", err );
    nvs_close( my_handle );
    return false;
  }

  // Close
  nvs_close( my_handle );
  return true;
}

void parameters_setDefaultValues( void )
{
  for ( uint8_t i = 0; i < sizeof( parameters_value ) / sizeof( uint32_t ); i++ )
  {
    parameters_value[i] = parameters[i].default_value;
  }
}

uint32_t parameters_getValue( parameter_value_t val )
{
  if ( val >= PARAM_LAST_VALUE )
  {
    return 0;
  }

  debug_function_name( "parameters_getValue" );
  return parameters_value[val];
}

uint32_t parameters_getMaxValue( parameter_value_t val )
{
  if ( val >= PARAM_LAST_VALUE )
  {
    return 0;
  }

  return parameters[val].max_value;
}

uint32_t parameters_getMinValue( parameter_value_t val )
{
  if ( val >= PARAM_LAST_VALUE )
  {
    return 0;
  }

  return parameters[val].min_value;
}

uint32_t parameters_getDefaultValue( parameter_value_t val )
{
  if ( val >= PARAM_LAST_VALUE )
  {
    return 0;
  }

  debug_function_name( "parameters_getDefaultValue" );
  return parameters[val].default_value;
}

bool parameters_setValue( parameter_value_t val, uint32_t value )
{
  if ( val >= PARAM_LAST_VALUE )
  {
    return false;
  }

  if ( value > parameters[val].max_value )
  {
    return false;
  }

  debug_function_name( "parameters_setValue" );
  parameters_value[val] = value;
  //ToDo send to Drv
  return true;
}

void parameters_init( void )
{
  bool ret_val = false;

  LOG( PRINT_INFO, "menu_param: system not started" );
  ret_val = _read_parameters();
  if ( ( ret_val == false ) || ( _check_values() == false ) )
  {
    LOG( PRINT_INFO, "menu_param: _read_parameters error %d", ret_val );
    parameters_setDefaultValues();
    parameters_save();
  }
  else
  {
    LOG( PRINT_INFO, "menu_param: _read_parameters success" );
  }
  parameters_debugPrint();
}

// bool parameters_RegisterCallback(parameter_value_t param, )
