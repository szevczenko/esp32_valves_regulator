/**
 *******************************************************************************
 * @file    cmd_wifi.h
 * @author  Dmytro Shevchenko
 * @brief   Command for WiFi driver
 *******************************************************************************
 */

#include "cmd_wifi.h"

#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "wifidrv.h"

#define JOIN_TIMEOUT_MS ( 10000 )

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static void _connect_cb( void )
{
  xEventGroupSetBits( wifi_event_group, CONNECTED_BIT );
}

static void _disconnect_cb( void )
{
  xEventGroupClearBits( wifi_event_group, CONNECTED_BIT );
}

static void initialise_wifi( void )
{
  static bool initialized = false;
  if ( initialized )
  {
    return;
  }
  wifiDrvRegisterConnectCb( _connect_cb );
  wifiDrvRegisterDisconnectCb( _disconnect_cb );
  initialized = true;
}

static bool wifi_join( const char* ssid, const char* pass, int timeout_ms )
{
  initialise_wifi();
  wifiDrvSetAPName( (char*) ssid, strlen( ssid ) );
  wifiDrvSetPassword( (char*) pass, strlen( pass ) );

  wifiDrvConnect();

  int bits = xEventGroupWaitBits( wifi_event_group, CONNECTED_BIT,
                                  pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS );
  return ( bits & CONNECTED_BIT ) != 0;
  return true;
}

/** Arguments used by 'join' function */
static struct
{
  struct arg_int* timeout;
  struct arg_str* ssid;
  struct arg_str* password;
  struct arg_end* end;
} join_args;

static int connect( int argc, char** argv )
{
  int nerrors = arg_parse( argc, argv, (void**) &join_args );
  if ( nerrors != 0 )
  {
    arg_print_errors( stderr, join_args.end, argv[0] );
    return 1;
  }
  ESP_LOGI( __func__, "Connecting to '%s'",
            join_args.ssid->sval[0] );

  /* set default value*/
  if ( join_args.timeout->count == 0 )
  {
    join_args.timeout->ival[0] = JOIN_TIMEOUT_MS;
  }

  bool connected = wifi_join( join_args.ssid->sval[0],
                              join_args.password->sval[0],
                              join_args.timeout->ival[0] );
  if ( !connected )
  {
    ESP_LOGW( __func__, "Connection timed out" );
    return 1;
  }
  ESP_LOGI( __func__, "Connected" );
  return 0;
}

void register_wifi( void )
{
  join_args.timeout = arg_int0( NULL, "timeout", "<t>", "Connection timeout, ms" );
  join_args.ssid = arg_str1( NULL, NULL, "<ssid>", "SSID of AP" );
  join_args.password = arg_str0( NULL, NULL, "<pass>", "PSK of AP" );
  join_args.end = arg_end( 2 );

  const esp_console_cmd_t join_cmd = {
    .command = "join",
    .help = "Join WiFi AP as a station",
    .hint = NULL,
    .func = &connect,
    .argtable = &join_args };

  ESP_ERROR_CHECK( esp_console_cmd_register( &join_cmd ) );
  wifi_event_group = xEventGroupCreate();
}
