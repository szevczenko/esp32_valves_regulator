/**
 *******************************************************************************
 * @file    parameters.h
 * @author  Dmytro Shevchenko
 * @brief   Parameters for working controller
 *******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/

#ifndef _PROJECT_PARAMETERS_H
#define _PROJECT_PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

/* Public types --------------------------------------------------------------*/

/* PARAM(param, min_value, max_value, default_value, name) */

#define PARAMETERS_U32_LIST                         \
  PARAM( PARAM_VALVE_1_STATE, 0, 1, 0 )             \
  PARAM( PARAM_VALVE_2_STATE, 0, 1, 0 )             \
  PARAM( PARAM_VALVE_3_STATE, 0, 1, 0 )             \
  PARAM( PARAM_VALVE_4_STATE, 0, 1, 0 )             \
  PARAM( PARAM_VALVE_5_STATE, 0, 1, 0 )             \
  PARAM( PARAM_VALVE_6_STATE, 0, 1, 0 )             \
  PARAM( PARAM_VALVE_7_STATE, 0, 1, 0 )             \
  PARAM( PARAM_ADD_WATER, 0, 1, 0 )                 \
  PARAM( PARAM_WATER_VOL_ADD, 0, UINT16_MAX, 100 )  \
  PARAM( PARAM_WATER_VOL_READ, 0, 10000, 0 )        \
  PARAM( PARAM_VOLTAGE_ACCUM, 0, UINT32_MAX, 0 )    \
  PARAM( PARAM_START_SYSTEM, 0, 1, 0 )              \
  PARAM( PARAM_SILOS_LEVEL, 0, 100, 0 )             \
  PARAM( PARAM_SILOS_SENSOR_IS_CONNECTED, 0, 1, 0 ) \
  PARAM( PARAM_SILOS_HEIGHT, 0, 300, 60 )           \
  PARAM( PARAM_LOW_LEVEL_SILOS, 0, 1, 0 )           \
  PARAM( PARAM_LANGUAGE, 0, 3, 0 )                  \
  PARAM( PARAM_PULSES_PER_LITER, 10, 10000, 100 )   \
  PARAM( PARAM_PWM_VALVE, 30, 100, 50 )             \
  PARAM( PARAM_TANK_SIZE, 100, 8000, 150 )          \
                                                    \
  /* ERRORS */                                      \
  PARAM( PARAM_MACHINE_ERRORS, 0, UINT32_MAX, 0 )   \
  PARAM( PARAM_WATER_FLOW_STATE, 0, 10, 0 )

#endif