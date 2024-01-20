#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "led.h"
#include "lwip/arch.h"

#ifndef SSD1306_I2C_PORT
#define SSD1306_I2C_PORT I2C_NUM_0
#endif

#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x3C
#endif

#define I2C_EXAMPLE_MASTER_SCL_IO 22 /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO 21 /*!< gpio number for I2C master data  */

// SSD1306 OLED height in pixels
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT 64
#endif

// SSD1306 width in pixels
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH 128
#endif

#define CFG_VALVE_CNT                    7
#define CFG_VALVE_CURRENT_REGULATION_PIN 27

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef TIMEOUT
#define TIMEOUT -2
#endif

#define T_WIFI_TYPE_SERVER 1
#define T_WIFI_TYPE_CLIENT 2

#define T_DEV_TYPE_SIEWNIK 1
#define T_DEV_TYPE_SOLARKA 2
#define T_DEV_TYPE_VALVE   3

#define LOGO_CLIENT_NAME "DEXWAL"

#define MENU_VIRO_ON_OFF_VERSION FALSE

///////////////////// LOGS //////////////////////

#define CONFIG_DEBUG_CMD_CLIENT        TRUE
#define CONFIG_DEBUG_CMD_SERVER        TRUE
#define CONFIG_DEBUG_PARSE_CMD         TRUE
#define CONFIG_DEBUG_WIFI              TRUE
#define CONFIG_DEBUG_BATTERY           TRUE
#define CONFIG_DEBUG_BUTTON            TRUE
#define CONFIG_DEBUG_ERROR_SIEWNIK     TRUE
#define CONFIG_DEBUG_KEEP_ALIVE        TRUE
#define CONFIG_DEBUG_MEASURE           TRUE
#define CONFIG_DEBUG_SERVER_CONTROLLER TRUE
#define CONFIG_DEBUG_MENU_BACKEND      TRUE
#define CONFIG_DEBUG_SLEEP             TRUE

/////////////////////  CONFIG PERIPHERALS  ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// CONSOLE
#define CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE 64
#define CONFIG_CONSOLE_SERIAL_SPEED        115200

///////////////////////////////////////////////////////////////////////////////////////////
//// LED
#define MOTOR_LED_RED         GPIO_NUM_15
#define SERVO_VIBRO_LED_RED   GPIO_NUM_12
#define MOTOR_LED_GREEN       GPIO_NUM_25
#define SERVO_VIBRO_LED_GREEN GPIO_NUM_26

// #define MOTOR_LED_SET_RED(x)            gpio_set_level(MOTOR_LED_RED, x);
// #define SERVO_VIBRO_LED_SET_RED(x)      gpio_set_level(SERVO_VIBRO_LED_RED, x);
// #define MOTOR_LED_SET_GREEN(x)          gpio_set_level(MOTOR_LED_GREEN, x);
// #define SERVO_VIBRO_LED_SET_GREEN(x)    gpio_set_level(SERVO_VIBRO_LED_GREEN, x);

#define MOTOR_LED_SET_RED( x )         set_motor_red_led( x );
#define SERVO_VIBRO_LED_SET_RED( x )   set_servo_red_led( x );
#define MOTOR_LED_SET_GREEN( x )       set_motor_green_led( x );
#define SERVO_VIBRO_LED_SET_GREEN( x ) set_servo_green_led( x );

//////////////////////////////////////  END  //////////////////////////////////////////////

#define NORMALPRIOR 5

#ifndef BOARD_NAME
#define BOARD_NAME "ESP-WROOM-32"
#endif

#ifndef BOARD_VERSION
#define BOARD_VERSION \
  {                   \
    1, 0, 0           \
  }
#endif

#ifndef SOFTWARE_VERSION
#define SOFTWARE_VERSION \
  {                      \
    1, 0, 0              \
  }
#endif

#ifndef DEFAULT_DEV_TYPE
#define DEFAULT_DEV_TYPE 1
#endif

#define CONFIG_BUFF_SIZE 512
#define ESP_OK           0
#define TIME_IMMEDIATE   0
#define NORMALPRIO       5

#define MS2ST( ms )   pdMS_TO_TICKS( ms )
#define ST2MS( tick ) ( ( tick ) * portTICK_PERIOD_MS )

#define osDelay( ms )       vTaskDelay( MS2ST( ms ) )
#define debug_printf( ... ) config_printf( __VA_ARGS__ )

enum config_print_lvl
{
  PRINT_DEBUG,
  PRINT_INFO,
  PRINT_WARNING,
  PRINT_ERROR,
  PRINT_TOP,
};

typedef struct
{
  const uint32_t start_config;
  uint32_t can_id;
  char name[32];
  uint8_t hw_ver[3];
  uint8_t sw_ver[3];
  uint8_t dev_type;
  uint8_t wifi_type;
  const uint32_t end_config;
} config_t;

typedef int esp_err_t;

extern config_t config;

int configSave( config_t* config );
int configRead( config_t* config );

void telnetPrintfToAll( const char* format, ... );
void telnetSendToAll( const char* data, size_t size );

void config_printf( enum config_print_lvl module_lvl, enum config_print_lvl msg_lvl, const char* format, ... );

void configInit( void );
void debug_function_name( const char* name );

#endif /* CONFIG_H_ */
