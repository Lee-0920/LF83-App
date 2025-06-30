/*
 * PMTDriver.h
 *
 *  Created on: 2019年11月4日
 *      Author: Administrator
 */

#ifndef SRC_DRIVER_OPTICALDRIVER_PMTDRIVER_H_
#define SRC_DRIVER_OPTICALDRIVER_PMTDRIVER_H_

#include "stm32f4xx.h"
#include "Common/Types.h"

void PMTDriver_Init(void);
void PMTDriver_PowerPinInit(void);
void PMTDriver_PowerOn(void);
void PMTDriver_PowerOff(void);
void PMTDriver_InitRef(void);
void PMTDriver_InitMea(void);
void PMTDriver_EnableRef(void);
void PMTDriver_DisableRef(void);
void PMTDriver_EnableMea(void);
void PMTDriver_DisableMea(void);
Uint32 PMTDriver_ReadCounterRef(void);
Uint32 PMTDriver_ReadCounterMea(void);

#endif /* SRC_DRIVER_OPTICALDRIVER_PMTDRIVER_H_ */
