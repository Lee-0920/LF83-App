/*
 * ThermostatDeviceMap.h
 *
 *  Created on: 2017年11月16日
 *      Author: LIANG
 */

#ifndef SRC_DRIVER_TEMPDRIVER_THERMOSTATDEVICEMAP_H_
#define SRC_DRIVER_TEMPDRIVER_THERMOSTATDEVICEMAP_H_

#include "ThermostatDevice.h"

#define MEAROOM_COOLER     0
#define MEAROOM_HEATER                5
#define BACTROOM_COOLER         1
#define CONDENSATOR_FAN_0               2
#define CONDENSATOR_FAN_1               3
#define BOX_FAN                4

#define TOTAL_THERMOSTATDEVICE          6

void ThermostatDeviceMap_Init(ThermostatDevice* device);
char* ThermostatDeviceMap_GetName(Uint8 index);

#endif /* SRC_DRIVER_TEMPDRIVER_THERMOSTATDEVICEMAP_H_ */
