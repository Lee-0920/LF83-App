/*
 * ValveManager.h
 *
 *  Created on: 2016年6月7日
 *      Author: Administrator
 */

#ifndef SRC_SOLENOIDVALVE_VALVEMANAGER_H_
#define SRC_SOLENOIDVALVE_VALVEMANAGER_H_

#include "Common/Types.h"

#define SOLENOIDVALVECONF_TOTALVAlVES         10
#define SOLENOIDVALVE_MAX_MAP                 0x3FF

void ValveManager_Init(void);
Uint16 ValveManager_GetTotalValves(void);
Bool ValveManager_SetValvesMap(Uint32 map);
Uint32 ValveManager_GetValvesMap(void);

#endif /* SRC_SOLENOIDVALVE_VALVEMANAGER_H_ */
