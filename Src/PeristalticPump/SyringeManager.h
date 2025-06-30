
#ifndef SRC_PERISTALTICPUMP_SYRINGEMANAGER_H_
#define SRC_PERISTALTICPUMP_SYRINGEMANAGER_H_

#include "Common/Types.h"
#include "StepperMotor.h"
#include "Driver/LiquidDriver/PositionSensor.h"
#include "FreeRTOS.h"
#include "task.h"

#define SYRINGE_PUMP_NUM         0        //用于注射器的泵号

typedef enum
{
    SYRINGE_IDLE,
    SYRINGE_EXTRACT,
    SYRINGE_DRAIN,
    SYRINGE_DRAIN_TO_SENSOR,
    SYRINGE_DRAIN_TO_ZERO
}SyringeStatus;

typedef struct
{
    StepperMotor stepperMotor;
    float factor;
    Bool isStatusSwitchStart;
    SyringeStatus status;
    __IO Bool isRequestStop;
    Bool isSendEvent;
    PositionSensor positionSensor;//检测高电平代表传感器被遮挡
    xTaskHandle xHandle;
    Uint32 limitStepCount;      // 限制步数计数
}Syringe;

void SyringeManager_Init();
void SyringeManager_Restore();
StepperMotorParam SyringeManager_GetDefaultMotionParam();
StepperMotorParam SyringeManager_GetCurrentMotionParam();
Uint16 SyringeManager_SetDefaultMotionParam(StepperMotorParam param);
Uint16 SyringeManager_SetCurrentMotionParam(StepperMotorParam param);
float SyringeManager_GetFactor();
Uint16 SyringeManager_SetFactor(float factor);
Uint16 SyringeManager_Start(Direction dir, float volume, Bool isUseDefaultParam);
Uint16 SyringeManager_RequestStop();
Bool SyringeManager_SendEventOpen();
Bool SyringeManager_IsSensorBlocked();
SyringeStatus SyringeManager_GetStatus();
Bool SyringeManager_IsWaterCheckSensorBlocked(Uint8 index);
StepperMotor* SyringeManager_GetStepperMotor();

#endif
