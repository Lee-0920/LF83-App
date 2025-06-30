/*
 * StaticADControl.h
 *
 *  Created on: 2020年1月6日
 *      Author: Administrator
 */

#ifndef SRC_OPTICALCONTROL_STATICADCONTROL_H_
#define SRC_OPTICALCONTROL_STATICADCONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include "Driver/System.h"

#define AD_CONTROLLER_NUM   2

#define PMT_REF     0        //REFC
#define PMT_MEA     1        //MEAC

void StaticADControl_InitDriver(void);
void StaticADControl_InitParam(void);
void StaticADControl_InitSetting(void);
Bool StaticADControl_LoadSetting(void);
void StaticADControl_Init(void);
Bool StaticADControl_Start(Uint8 index, Uint32 targetAD);
void StaticADControl_Stop(void);
Uint16 StaticADControl_GetDefaultValue(Uint8 index);
void StaticADControl_SetDefaultValue(Uint8 index, Uint16 value);
Uint16 StaticADControl_GetRealValue(Uint8 index);
Bool StaticADControl_SetRealValue(Uint8 index, Uint16 value);
void StaticADControl_SendEventOpen(void);
void StaticADControl_SendEventClose(void);
Bool StaticADControl_IsValid(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_OPTICALCONTROL_STATICADCONTROL_H_ */
