#ifndef START_MENU_H_
#define START_MENU_H_
#include "app_config.h"
#include "error_valve.h"
#include "menu_drv.h"

struct valve_data
{
  bool state;
  uint32_t flow;
};

struct menu_data
{
  uint32_t water_volume_l;
  struct valve_data valve[CFG_VALVE_CNT];
};

void menuInitStartMenu( menu_token_t* menu );
void menuStartReset( void );
void menuStartSetError( error_type_t error );
void menuStartResetError( void );
struct menu_data* menuStartGetData( void );

#endif