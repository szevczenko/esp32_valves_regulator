#include "menu_param.h"

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

#define STORAGE_NAMESPACE   "MENU"
#define PARAMETERS_TAB_SIZE MENU_LAST_VALUE

static menuPStruct_t menuParameters[] =
  {
    [MENU_VALVE_1_STATE] = {.max_value = 1,       .default_value = 0  },
    [MENU_VALVE_2_STATE] = { .max_value = 1,      .default_value = 0  },
    [MENU_VALVE_3_STATE] = { .max_value = 1,      .default_value = 0  },
    [MENU_VALVE_4_STATE] = { .max_value = 1,      .default_value = 0  },
    [MENU_VALVE_5_STATE] = { .max_value = 1,      .default_value = 0  },
    [MENU_VALVE_6_STATE] = { .max_value = 1,      .default_value = 0  },
    [MENU_VALVE_7_STATE] = { .max_value = 1,      .default_value = 0  },
    [MENU_ADD_WATER] = { .max_value = 1,      .default_value = 0  },
    [MENU_WATER_VOL_ADD] = { .max_value = 2500,   .default_value = 100},
    [MENU_WATER_VOL_READ] = { .max_value = 10000,  .default_value = 0  },
    [MENU_VOLTAGE_ACCUM] = { .max_value = 1,      .default_value = 0  },
    [MENU_START_SYSTEM] = { .max_value = 1,      .default_value = 0  },
    [MENU_BOOTUP_SYSTEM] = { .max_value = 1,      .default_value = 1  },
    [MENU_EMERGENCY_DISABLE] = { .max_value = 1,      .default_value = 0  },
    [MENU_SILOS_LEVEL] = { .max_value = 100,    .default_value = 0  },
    [MENU_SILOS_HEIGHT] = { .max_value = 300,    .default_value = 60 },
    [MENU_LOW_LEVEL_SILOS] = { .max_value = 1,      .default_value = 0  },
    [MENU_SILOS_SENSOR_IS_CONECTED] = { .max_value = 1,      .default_value = 0  },
    [MENU_LANGUAGE] = { .max_value = 3,      .default_value = 0  },
    [MENU_POWER_ON_MIN] = { .max_value = 100,    .default_value = 30 },

    [MENU_MACHINE_ERRORS] = { .max_value = 0xFFFF, .default_value = 0  },

    [MENU_BUZZER] = { .max_value = 1,      .default_value = 1  },
    [MENU_BRIGHTNESS] = { .max_value = 10,     .default_value = 10 },
};

static char* parameters_name[] =
  {
    [MENU_VALVE_1_STATE] = "MENU_VALVE_1_STATE",
    [MENU_VALVE_2_STATE] = "MENU_VALVE_2_STATE",
    [MENU_VALVE_3_STATE] = "MENU_VALVE_3_STATE",
    [MENU_VALVE_4_STATE] = "MENU_VALVE_4_STATE",
    [MENU_VALVE_5_STATE] = "MENU_VALVE_5_STATE",
    [MENU_VALVE_6_STATE] = "MENU_VALVE_6_STATE",
    [MENU_VALVE_7_STATE] = "MENU_VALVE_7_STATE",
    [MENU_ADD_WATER] = "MENU_ADD_WATER",
    [MENU_WATER_VOL_ADD] = "MENU_WATER_VOL_ADD",
    [MENU_WATER_VOL_READ] = "MENU_WATER_VOL_READ",
    [MENU_VOLTAGE_ACCUM] = "MENU_VOLTAGE_ACCUM",
    [MENU_START_SYSTEM] = "MENU_START_SYSTEM",
    [MENU_BOOTUP_SYSTEM] = "MENU_BOOTUP_SYSTEM",
    [MENU_EMERGENCY_DISABLE] = "MENU_EMERGENCY_DISABLE",
    [MENU_LANGUAGE] = "MENU_LANGUAGE",
    [MENU_POWER_ON_MIN] = "MENU_POWER_ON_MIN",

    [MENU_MACHINE_ERRORS] = "MENU_MACHINE_ERRORS",

    /* calibration value */
    [MENU_BUZZER] = "MENU_BUZZER",
    [MENU_BRIGHTNESS] = "MENU_BRIGHTNESS",
};

RTC_NOINIT_ATTR uint32_t menuSaveParameters_data[MENU_LAST_VALUE];

void menuPrintParameters( void )
{
  for ( uint8_t i = 0; i < PARAMETERS_TAB_SIZE; i++ )
  {
    LOG( PRINT_DEBUG, "%s : %d\n", parameters_name[i], menuSaveParameters_data[i] );
  }
}

void menuPrintParameter( menuValue_t val )
{
  if ( val >= MENU_LAST_VALUE )
  {
    return;
  }

  LOG( PRINT_DEBUG, "Param: %s : %d", parameters_name[val], menuSaveParameters_data[val] );
}

esp_err_t menuSaveParameters( void )
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle );
  if ( err != ESP_OK )
  {
    LOG( PRINT_INFO, "nvs_open error %d", err );
    nvs_close( my_handle );
    return err;
  }

  err = nvs_set_blob( my_handle, "menu", menuSaveParameters_data, sizeof( menuSaveParameters_data ) );

  if ( err != ESP_OK )
  {
    LOG( PRINT_INFO, "nvs_set_blob error %d", err );
    nvs_close( my_handle );
    return err;
  }

  // Commit
  err = nvs_commit( my_handle );
  if ( err != ESP_OK )
  {
    LOG( PRINT_INFO, "nvs_commit error %d", err );
    nvs_close( my_handle );
    return err;
  }

  // Close
  nvs_close( my_handle );
  LOG( PRINT_INFO, "menuSaveParameters success" );
  return ESP_OK;
}

esp_err_t menuReadParameters( void )
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle );
  if ( err != ESP_OK )
  {
    return err;
  }

  // Read the size of memory space required for blob
  size_t required_size = 0;    // value will default to 0, if not set yet in NVS

  err = nvs_get_blob( my_handle, "menu", NULL, &required_size );
  if ( ( err != ESP_OK ) && ( err != ESP_ERR_NVS_NOT_FOUND ) )
  {
    nvs_close( my_handle );
    return err;
  }

  // Read previously saved blob if available
  if ( required_size == sizeof( menuSaveParameters_data ) )
  {
    err = nvs_get_blob( my_handle, "menu", menuSaveParameters_data, &required_size );
    nvs_close( my_handle );
    return err;
  }

  nvs_close( my_handle );
  return ESP_ERR_NVS_NOT_FOUND;
}

static void menuSetDefaultForReadValue( void )
{
  menuSaveParameters_data[MENU_MACHINE_ERRORS] = 0;
  menuSaveParameters_data[MENU_START_SYSTEM] = 0;
}

void menuSetDefaultValue( void )
{
  for ( uint8_t i = 0; i < sizeof( menuSaveParameters_data ) / sizeof( uint32_t ); i++ )
  {
    menuSaveParameters_data[i] = menuParameters[i].default_value;
  }
}

int menuCheckValues( void )
{
  for ( uint8_t i = 0; i < sizeof( menuSaveParameters_data ) / sizeof( uint32_t ); i++ )
  {
    if ( menuSaveParameters_data[i] > menuGetMaxValue( i ) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

uint32_t menuGetValue( menuValue_t val )
{
  if ( val >= MENU_LAST_VALUE )
  {
    return 0;
  }

  debug_function_name( "menuGetValue" );
  return menuSaveParameters_data[val];
}

uint32_t menuGetMaxValue( menuValue_t val )
{
  if ( val >= MENU_LAST_VALUE )
  {
    return 0;
  }

  debug_function_name( "menuGetMaxValue" );
  return menuParameters[val].max_value;
}

uint32_t menuGetDefaultValue( menuValue_t val )
{
  if ( val >= MENU_LAST_VALUE )
  {
    return 0;
  }

  debug_function_name( "menuGetDefaultValue" );
  return menuParameters[val].default_value;
}

uint8_t menuSetValue( menuValue_t val, uint32_t value )
{
  if ( val >= MENU_LAST_VALUE )
  {
    return FALSE;
  }

  if ( value > menuParameters[val].max_value )
  {
    return FALSE;
  }

  debug_function_name( "menuSetValue" );
  menuSaveParameters_data[val] = value;
  //ToDo send to Drv
  return TRUE;
}

void menuParamGetDataNSize( void** data, uint32_t* size )
{
  if ( ( data == NULL ) || ( size == NULL ) )
  {
    return;
  }

  debug_function_name( "menuParamGetDataNSize" );
  *data = (void*) menuSaveParameters_data;
  *size = sizeof( menuSaveParameters_data );
}

void menuParamSetDataNSize( void* data, uint32_t size )
{
  if ( data == NULL )
  {
    return;
  }

  if ( sizeof( menuSaveParameters_data ) != size )
  {
    LOG( PRINT_ERROR, "%s Bad long", __func__ );
    return;
  }

  memcpy( menuSaveParameters_data, data, size );
}

void menuParamInit( void )
{
  int ret_val = 0;

  if ( ( menuCheckValues() == FALSE ) || ( menuGetValue( MENU_START_SYSTEM ) == 0 ) )
  {
    LOG( PRINT_INFO, "menu_param: system not started" );
    ret_val = menuReadParameters();
    if ( ret_val != ESP_OK )
    {
      LOG( PRINT_INFO, "menu_param: menuReadParameters error %d", ret_val );
      menuSetDefaultValue();
    }
    else
    {
      LOG( PRINT_INFO, "menu_param: menuReadParameters succes" );
      menuPrintParameters();
      menuSetDefaultForReadValue();
    }
  }
  else
  {
    LOG( PRINT_INFO, "menu_param: system started" );
    //menuPrintParameters();
  }

  menuSetValue( MENU_VALVE_1_STATE, 0 );
  menuSetValue( MENU_VALVE_2_STATE, 0 );
  menuSetValue( MENU_VALVE_3_STATE, 0 );
  menuSetValue( MENU_VALVE_4_STATE, 0 );
  menuSetValue( MENU_VALVE_5_STATE, 0 );
  menuSetValue( MENU_VALVE_6_STATE, 0 );
  menuSetValue( MENU_VALVE_7_STATE, 0 );
  menuSetValue( MENU_ADD_WATER, 0 );
  menuSetValue( MENU_WATER_VOL_ADD, 0 );
  menuSetValue( MENU_WATER_VOL_READ, 0 );
}
