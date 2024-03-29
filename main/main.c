#include <stdio.h>

#include "app_config.h"
#include "battery.h"
#include "but.h"
#include "buzzer.h"
#include "cmd_client.h"
#include "cmd_server.h"
#include "dictionary.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "error_valve.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "fast_add.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "intf/i2c/ssd1306_i2c.h"
#include "keepalive.h"
#include "measure.h"
#include "menu_backend.h"
#include "menu_drv.h"
#include "mongoose_drv.h"
#include "nvs_flash.h"
#include "oled.h"
#include "ota_drv.h"
#include "parameters.h"
#include "pcf8574.h"
#include "power_on.h"
#include "prod_app.h"
#include "server_controller.h"
#include "sleep_e.h"
#include "ssd1306.h"
#include "wifidrv.h"

#ifndef CFG_PRODUCTION_APPLICATION
#define CFG_PRODUCTION_APPLICATION 0
#endif

extern void ultrasonar_start( void );

static gpio_config_t io_conf;
static uint32_t blink_pin = GPIO_NUM_23;
portMUX_TYPE portMux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t wifi_type;

static bool check_i2c_communication( void )
{
  ssd1306_i2cInitEx( I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, SSD1306_I2C_ADDR );
  uint8_t s_i2c_addr = 0x3C;
  int ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start( cmd );
  i2c_master_write_byte( cmd, ( s_i2c_addr << 1 ) | I2C_MASTER_WRITE, 0x1 );
  i2c_master_write_byte( cmd, 0x00, 0x1 );
  i2c_master_stop( cmd );
  ret = i2c_master_cmd_begin( I2C_NUM_1, cmd, MS2ST( 1000 ) );
  i2c_cmd_link_delete( cmd );
  printf( "I2C TEST %d\n\r", ret );
  return ret == ESP_OK;
}

void graphic_init( void )
{
  sh1106_128x64_init();
  ssd1306_clearScreen();
  ssd1306_fillScreen( 0x00 );
  ssd1306_flipHorizontal( 1 );
  ssd1306_flipVertical( 1 );
  oled_init();
}

static void checkDevType( void )
{
  pcf8574_init();
  bool read_i2c_value = check_i2c_communication();
  if ( read_i2c_value )
  {
    wifi_type = T_WIFI_TYPE_CLIENT;
  }
  else
  {
    wifi_type = T_WIFI_TYPE_SERVER;
  }
  wifiDrvSetWifiType( wifi_type );
}

void app_init( void )
{
  nvs_flash_init();
  DevConfig_Init();
  OTA_Init();
}

static void _toggle_emergency_disable( void )
{
  backendToggleEmergencyDisable();
}

static void _init_remote_controller( void )
{
  graphic_init();
  menuBackendInit();
  battery_init();
  init_leds();
  osDelay( 10 );

  buzzer_init();
  power_on_init();

  /* Wait to measure voltage */
  while ( !battery_is_measured() )
  {
    osDelay( 10 );
  }

  float voltage = battery_get_voltage();
  power_on_enable_system();
  init_buttons();

  if ( voltage > 3.2 )
  {
    menuDrvInit( MENU_DRV_NORMAL_INIT, _toggle_emergency_disable );
    wifiDrvInit();
    cmdClientStartTask();
    keepAliveStartTask();
    dictionary_init();
    fastProcessStartTask();
    power_on_start_task();
    // init_sleep();
  }
  else
  {
    menuDrvInit( MENU_DRV_LOW_BATTERY_INIT, _toggle_emergency_disable );
    power_on_disable_system();
  }
}

static void _init_server_controller( void )
{
  wifiDrvInit();

  keepAliveStartTask();
  cmdServerStartTask();
  measure_start();
  srvrControllStart();
  ultrasonar_start();
  //WYLACZONE

  errorStart();

  //LED on
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = ( 1 << blink_pin );
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config( &io_conf );
  gpio_set_level( blink_pin, 1 );
  // MongooseDrv_Init();
}

void MainApp_Start( void )
{
  app_init();
  checkDevType();
  parameters_init();

  if ( wifi_type != T_WIFI_TYPE_SERVER )
  {
    _init_remote_controller();
  }
  else
  {
    _init_server_controller();
  }

  DevConfig_Printf( PRINT_DEBUG, PRINT_DEBUG, "[MENU] ------------START SYSTEM-------------" );
  DevConfig_Printf( PRINT_DEBUG, PRINT_DEBUG, "[MENU] SN %s", DevConfig_GetSerialNumber() );
  while ( 1 )
  {
    vTaskDelay( MS2ST( 250 ) );
    if ( ( wifi_type == T_WIFI_TYPE_SERVER ) && !cmdServerIsWorking() )
    {
      gpio_set_level( blink_pin, 0 );
    }

    vTaskDelay( MS2ST( 750 ) );

    if ( wifi_type == T_WIFI_TYPE_SERVER )
    {
      gpio_set_level( blink_pin, 1 );
    }
  }
}

void app_main( void )
{
#if CFG_PRODUCTION_APPLICATION
  ProdApp_Start();
#else
  MainApp_Start();
#endif
}
