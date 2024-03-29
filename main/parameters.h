/**
 *******************************************************************************
 * @file    parameters.h
 * @author  Dmytro Shevchenko
 * @brief   Parameters for working controller
 *******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

/* Public types --------------------------------------------------------------*/

typedef void ( *param_set_cb )( void* user_data, uint32_t value );

typedef enum
{
  PARAM_VALVE_1_STATE,
  PARAM_VALVE_2_STATE,
  PARAM_VALVE_3_STATE,
  PARAM_VALVE_4_STATE,
  PARAM_VALVE_5_STATE,
  PARAM_VALVE_6_STATE,
  PARAM_VALVE_7_STATE,
  PARAM_ADD_WATER,
  PARAM_WATER_VOL_ADD,
  PARAM_WATER_VOL_READ,
  PARAM_VOLTAGE_ACCUM,
  PARAM_START_SYSTEM,
  PARAM_BOOT_UP_SYSTEM,
  PARAM_EMERGENCY_DISABLE,
  PARAM_SILOS_LEVEL,
  PARAM_SILOS_SENSOR_IS_CONNECTED,
  PARAM_SILOS_HEIGHT,
  PARAM_LOW_LEVEL_SILOS,
  PARAM_LANGUAGE,
  PARAM_POWER_ON_MIN,
  PARAM_PULSES_PER_LITER,
  PARAM_PWM_VALVE,
  PARAM_TANK_SIZE,

  /* ERRORS */
  PARAM_MACHINE_ERRORS,
  PARAM_WATER_FLOW_STATE,

  /* calibration value */
  PARAM_BUZZER,
  PARAM_BRIGHTNESS,
  PARAM_LAST_VALUE

} parameter_value_t;

typedef enum
{
  PARAM_STR_CONTROLLER_SN,
  PARAM_STR_LAST_VALUE
} parameter_string_t;

typedef struct
{
  uint32_t min_value;
  uint32_t max_value;
  uint32_t default_value;
  const char* name;

  void* user_data;
  param_set_cb cb;
} parameter_t;

/* Public functions ----------------------------------------------------------*/

/**
 * @brief   Init parameters.
 */
void parameters_init( void );

/**
 * @brief   Save parameters in nvm.
 * @return  true - if success
 */
bool parameters_save( void );

/**
 * @brief   Set default values.
 */
void parameters_setDefaultValues( void );

/**
 * @brief   Get value.
 * @param   [in] val - parameter which get value
 * @return  value of parameter
 */
uint32_t parameters_getValue( parameter_value_t val );

/**
 * @brief   Get max value.
 * @param   [in] val - parameter which get value
 * @return  value of parameter
 */
uint32_t parameters_getMaxValue( parameter_value_t val );

/**
 * @brief   Get min value.
 * @param   [in] val - parameter which get value
 * @return  value of parameter
 */
uint32_t parameters_getMinValue( parameter_value_t val );

/**
 * @brief   Get default values.
 * @param   [in] val - parameter which get value
 * @return  value of parameter
 */
uint32_t parameters_getDefaultValue( parameter_value_t val );

/**
 * @brief   Set values.
 * @param   [in] val - parameter which set value
 * @return  true - if success
 */
bool parameters_setValue( parameter_value_t val, uint32_t value );

/**
 * @brief   Set string.
 * @param   [in] val - parameter which set value
 * @param   [in] str - set string value
 * @return  true - if success
 */
bool parameters_setString( parameter_string_t val, const char* str );

/**
 * @brief   Set string.
 * @param   [in] val - parameter which set value
 * @param   [out] str - buffer to copy string
 * @param   [in] str_len - buffer size
 * @return  true - if success
 */
bool parameters_getString( parameter_string_t val, char* str, uint32_t str_len );

/**
 * @brief   Print in serial all parameters
 */
void parameters_debugPrint( void );

/**
 * @brief   Print in serial parameter value
 * @param   [in] val - parameter which print value
 */
void parameters_debugPrintValue( parameter_value_t val );

#endif