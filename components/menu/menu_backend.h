#ifndef MENU_BACKEND_H_
#define MENU_BACKEND_H_

void menuBackendInit(void);
void backendEnterparameterseters(void);
void backendExitparameterseters(void);
void backendToggleEmergencyDisable(void);
void backendEnterMenuStart(void);
void backendExitMenuStart(void);
bool backendIsConnected(void);
bool backendIsEmergencyDisable(void);

void backendSetWater(bool on_off);
#endif