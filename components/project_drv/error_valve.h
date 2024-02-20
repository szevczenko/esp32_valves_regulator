#ifndef _ERROR_VALVE_H_
#define _ERROR_VALVE_H_
#include "app_config.h"

#define ERROR_CRITICAL_VOLTAGE 800
#define ERROR_M_TIME_EXIT 2000

#define SERVO_WAIT_TO_TRY 1500
#define SERVO_WAIT_AFTER_TRY 2000
#define SERVO_TRY_CNT 3

#define MOTOR_RESISTOR 0.033

typedef enum
{
	ERROR_TOP
} error_type_t;

void errorStart(void);
void errorReset(void);

#endif