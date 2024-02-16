#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <stdbool.h>

#define LANGUAGE_CNT_SUPPORT 4

typedef enum
{
    DICT_LANGUAGE_ENGLISH,
    DICT_LANGUAGE_RUSSIAN,
    DICT_LANGUAGE_POLISH,
    DICT_LANGUAGE_GERMANY,
    DICT_LANGUAGE_TOP
} dict_language_t;

enum dictionary_phrase
{
    /* Bootup */
    DICT_LOGO_CLIENT_NAME,
    DICT_INIT,
    DICT_WAIT_TO_START_WIFI,
    DICT_TRY_CONNECT_TO_S,
    DICT_WAIT_CONNECTION_S_S_S,
    DICT_CONNECTED_TRY_READ_DATA,
    DICT_READ_DATA_FROM_S,
    DICT_SYSTEM_READY_TO_START,
    /* MENU */
    DICT_SETTINGS,
    DICT_START,
    DICT_DEVICES,
    DICT_PARAMETES,
    DICT_LOW_BAT,
    DICT_MENU,
    /* MENU DRV */
    DICT_WAIT_TO_INIT,
    DICT_MENU_IDLE_STATE,
    DICT_MENU_STOP,
    DICT_POWER_OFF,
    /* SETTINGS */
    DICT_BOOTING,
    DICT_BUZZER,
    DICT_LANGUAGE,
    DICT_IDLE_TIME,
    DICT_MOTOR_ERR,
    DICT_SERVO_ERR,
    DICT_VIBRO_ERR,
    DICT_PERIOD,
    DICT_BRIGHTNESS,
    DICT_MOTOR_ERROR_CALIBRATION,
    DICT_SERVO_CLOSE,
    DICT_SERVO_OPEN,
    DICT_ON,
    DICT_OFF,
    /* PARAMETERS */
    DICT_CURRENT,
    DICT_VOLTAGE,
    DICT_SIGNAL,
    DICT_TEMP,
    DICT_CONNECT,
    DICT_DEVICE_NOT_CONNECTED,
    /* WIFI */
    DICT_WAIT_TO_WIFI_INIT,
    DICT_SCANNING_DEVICES,
    DICT_CLICK_ENTER_TO_SCANNING,
    DICT_FIND_DEVICES,
    DICT_DEVICE_NOT_FOUND,
    DICT_TRY_CONNECT_TO,
    DICT_WAIT_TO_CONNECT,
    DICT_WIFI_CONNECTED,
    DICT_WAIT_TO_SERVER,
    DICT_ERROR_CONNECT,
    DICT_CONNECTED_TO,
    /* LOW BATTERY */
    DICT_LOW_BATTERY,
    DICT_CONNECT_CHARGER,
    DICT_CHECK_CONNECTION,
    DICT_VIBRO_ON,
    DICT_VIBRO_OFF,
    DICT_LOW,
    DICT_SILOS,
    DICT_MOTOR,
    DICT_SERVO,
    DICT_TARGET_NOT_CONNECTED,
    DICT_POWER_SAVE,
    DICT_SERVO_NOT_CONNECTED,
    DICT_SERVO_OVERCURRENT,
    DICT_MOTOR_NOT_CONNECTED,
    DICT_VIBRO_NOT_CONNECTED,
    DICT_VIBRO_OVERCURRENT,
    DICT_MOTOR_OVERCURRENT,
    DICT_TEMPERATURE_IS_HIGH,
    DICT_UNKNOWN_ERROR,
    DICT_LOST_CONNECTION_WITH_SERVER,
    DICT_TIMEOUT_CONNECT,
    DICT_TIMEOUT_SERVER,
    DICT_PULSES_PER_LITER,
    DICT_PWM_VALVE,
    DICT_TANK_SIZE,
    DICT_TOP
};

void dictionary_init(void);
bool dictionary_set_language(dict_language_t lang);
const char *dictionary_get_string(enum dictionary_phrase phrase);

#endif