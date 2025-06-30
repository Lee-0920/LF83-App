/*
 * PeristalticPumpManager.h
 *
 *  Created on: 2016年5月31日
 *      Author: Administrator
 */

#ifndef SRC_PERISTALTICPUMP_PERISTALTICPUMPMANAGER_H_
#define SRC_PERISTALTICPUMP_PERISTALTICPUMPMANAGER_H_


#include "StepperMotor.h"
#include "Common/Types.h"
#include "string.h"
#include "tracer/trace.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PERISTALTICPUMPMANAGER_TOTAL_PUMP 1

#define PUMP_STEP_MOTOR_OFFSET   3  //蠕动泵编号 + 3  = 步进电机编号

typedef struct
{
    StepperMotor stepperMotor;
    float factor;
    Bool isSendEvent;
}PeristalticPump;

void PeristalticPumpManager_Init(void);
void PeristalticPumpManager_Restore(void);
Uint16 PeristalticPumpManager_GetTotalPumps(void);
StepperMotorParam PeristalticPumpManager_GetDefaultMotionParam(Uint8 index);
StepperMotorParam PeristalticPumpManager_GetCurrentMotionParam(Uint8 index);
Uint16 PeristalticPumpManager_SetDefaultMotionParam(Uint8 index, StepperMotorParam param);
Uint16 PeristalticPumpManager_SetCurrentMotionParam(Uint8 index, StepperMotorParam param);
float PeristalticPumpManager_GetFactor(Uint8 index);
Uint16 PeristalticPumpManager_SetFactor(Uint8 index, float factor);
StepperMotorStatus PeristalticPumpManager_GetStatus(Uint8 index);
float PeristalticPumpManager_GetVolume(Uint8 index);
Uint16 PeristalticPumpManager_Start(Uint8 index, Direction dir, float volume, Bool isUseDefaultParam);
Uint16 PeristalticPumpManager_Stop(Uint8 index);
void PeristalticPumpManager_ChangeVolume(Uint8 index, float volume);
Bool PeristalticPumpManager_SendEventOpen(Uint8 index);
StepperMotor* PeristalticPumpManager_GetStepperMotor(Uint8 index);

#ifdef __cplusplus
}
#endif

#endif /* SRC_PERISTALTICPUMP_PERISTALTICPUMPMANAGER_H_ */
