#ifndef MENU_PARAM_H
#define MENU_PARAM_H

#include "config.h"
#include "dictionary.h"

typedef enum
{
	MENU_VALVE_1_STATE,
	MENU_VALVE_2_STATE,
	MENU_VALVE_3_STATE,
	MENU_VALVE_4_STATE,
	MENU_VALVE_5_STATE,
	MENU_VALVE_6_STATE,
	MENU_VALVE_7_STATE,
	MENU_VOLTAGE_ACCUM,
	MENU_START_SYSTEM,
	MENU_BOOTUP_SYSTEM,
	MENU_EMERGENCY_DISABLE,
	MENU_LANGUAGE,
	MENU_POWER_ON_MIN,

	/* ERRORS */
	MENU_MACHINE_ERRORS,

	/* calibration value */
	MENU_BUZZER,
	MENU_BRIGHTNESS,
	MENU_LAST_VALUE

}menuValue_t;

void menuParamInit(void);
esp_err_t menuSaveParameters(void);
esp_err_t menuReadParameters(void);
void menuSetDefaultValue(void);
uint32_t menuGetValue(menuValue_t val);
uint32_t menuGetMaxValue(menuValue_t val);
uint32_t menuGetDefaultValue(menuValue_t val);
uint8_t menuSetValue(menuValue_t val, uint32_t value);
void menuParamGetDataNSize(void ** data, uint32_t * size);
void menuParamSetDataNSize(void * data, uint32_t size);

void menuPrintParameters(void);
void menuPrintParameter(menuValue_t val);

#endif