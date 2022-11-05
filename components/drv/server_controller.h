#ifndef SERVER_CONTROLLER_H_
#define SERVER_CONTROLLER_H_
#include "config.h"
#include "parse_cmd.h"

bool srvrControllGetEmergencyDisable(void);
void srvrControllStart(void);
bool srvrConrollerSetError(uint16_t error_reason);
bool srvrControllerErrorReset(void);
bool srvrControllIsWorking(void);

#endif