
/**
 *******************************************************************************
 * @file    prod_http_app.c
 * @author  Dmytro Shevchenko
 * @brief   Production HTTP app
 *******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/

#include "dev_config.h"
#include "mongoose.h"
#include "ota_drv.h"

/* Private macros ------------------------------------------------------------*/
#define HTTP_URL "http://0.0.0.0:8000"

/* Private functions declaration ---------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static void fn( struct mg_connection* c, int ev, void* ev_data )
{
  if ( ev == MG_EV_HTTP_MSG )
  {
    struct mg_http_message* hm = (struct mg_http_message*) ev_data;
    if ( mg_http_match_uri( hm, "/api/sn" ) )
    {
      if ( mg_vcasecmp( &hm->method, "PUT" ) == 0 || mg_vcasecmp( &hm->method, "POST" ) == 0 )
      {
        char sn[33] = { 0 };
        assert( hm->body.len <= sizeof( sn ) );
        memcpy( sn, hm->body.ptr, hm->body.len );
        assert( DevConfig_SetSerialNumber( sn ) );
        mg_http_reply( c, 200, "", "OK\n" );
      }
      else if ( mg_vcasecmp( &hm->method, "GET" ) == 0 )
      {
        const char* sn = DevConfig_GetSerialNumber();
        mg_http_reply( c, 200, "", "%s\n", sn );
      }
      else
      {
        mg_http_reply( c, 400, "", "FAIL\n" );
      }
    }
    else if ( mg_http_match_uri( hm, "/api/ota" ) )
    {
      if ( mg_vcasecmp( &hm->method, "PUT" ) == 0 || mg_vcasecmp( &hm->method, "POST" ) == 0 )
      {
        char url[256] = { 0 };
        assert( hm->body.len <= sizeof( url ) );
        memcpy( url, hm->body.ptr, hm->body.len );
        if ( OTA_Download( url ) )
        {
          mg_http_reply( c, 200, "", "OK\n" );
        }
        else
        {
          mg_http_reply( c, 500, "", "Internal Server Error\n" );
        }
      }
      else
      {
        mg_http_reply( c, 400, "", "FAIL\n" );
      }
    }
    else
    {
      mg_http_reply( c, 405, "", "Method Not Allowed\n" );
    }
  }
}

static void _task( void* argv )
{
  struct mg_mgr mgr;
  mg_mgr_init( &mgr );    // Init manager
  mg_log_set( MG_LL_DEBUG );    // Set log level
  mg_http_listen( &mgr, HTTP_URL, fn, &mgr );    // Setup listener
  for ( ;; )
    mg_mgr_poll( &mgr, 1000 );    // Event loop
  mg_mgr_free( &mgr );    // Cleanup
}

/* Public functions ---------------------------------------------------------*/

void MongooseHTTPApp_Init( void )
{
  xTaskCreate( _task, "mongoose", 8096, NULL, 13, NULL );
}
