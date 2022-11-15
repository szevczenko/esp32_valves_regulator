#ifndef MENU_BACKEND_H_
#define MENU_BACKEND_H_

void menuBackendInit(void);
void backendEnterMenuParameters(void);
void backendExitMenuParameters(void);
void backendToggleEmergencyDisable(void);
void backendEnterMenuStart(void);
void backendExitMenuStart(void);
bool backendIsConnected(void);
bool backendIsEmergencyDisable(void);

void backendSetWater(bool on_off);
#endif