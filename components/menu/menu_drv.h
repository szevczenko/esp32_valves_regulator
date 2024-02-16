#ifndef MENU_DRV_H
#define MENU_DRV_H
#include "menu.h"
#include "oled.h"

int menuDrvElementsCnt(menu_token_t * menu);
void menuEnter(menu_token_t * menu);
void menuDrv_Exit(menu_token_t * menu);
void menuSetMain(menu_token_t * menu);
void menuDrvSaveParameters(void);
void menuPrintfInfo(const char *format, ...);
void menuDrvEnterEmergencyDisable(void);
void menuDrvExitEmergencyDisable(void);

void enterMenuStart(void);
void menuDrv_EnterToParameters(void);
void menuDrvDisableSystemProcess(void);

#endif