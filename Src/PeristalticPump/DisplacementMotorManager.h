/*
 * DisplacementMotorManager.h
 *
 *  Created on: 2018年3月7日
 *      Author: LIANG
 */

#ifndef SRC_PERISTALTICPUMP_DISPLACEMENTMOTORMANAGER_H_
#define SRC_PERISTALTICPUMP_DISPLACEMENTMOTORMANAGER_H_

#include "Common/Types.h"
#include "FreeRTOS.h"
#include "task.h"
#include "StepperMotor.h"
#include "Driver/LiquidDriver/PositionSensor.h"

#define DISPLACEMENTMOTOR_TOTAL_PUMP 2
#define X_DISPLACEMENTMOTOR    0
#define Z_DISPLACEMENTMOTOR    1

typedef enum
{
    MOTOR_MOVE_ABSOLUTE_MODE,//绝对模式
    MOTOR_MOVE_RELATIVE_MODE,//相对模式
    MOTOR_MOVE_SAFE_MODE,
    MAX_MOTOR_MOVE_MODE,
}DisplacementMotorMode;

typedef enum
{
    MOTOR_IDLE,
    MOTOR_TO_WAIT_Z_RESET,
    MOTOR_TO_TARGET_LOCATION,
    MOTOR_TO_SENSOR,
    MOTOR_TO_ZERO,
}DisplacementMotorStatus;

typedef void (*DisplacementMotor_MoveHandler)(void* obj);

typedef struct
{
    StepperMotor stepperMotor;
    PositionSensor positionSensor;
    //PositionSensor limitSensor;
    Int32 currentSteps;//当前位置
    Uint32 maxSteps;//单次启动的最大运行步数
    __IO Bool isRequestStop;
    Uint32 limitStepCount;      // 限制步数计数
    Int32 returnStepCount;     // 回归预测步数
    Bool isChangeSteps;
    Bool isSendEvent;
    Bool isStatusSwitchStart;
    DisplacementMotorStatus status;
    DisplacementMotorMode mode;
    Direction dir;
    Int32 targetStep;
    Bool isUseDefaultParam;
    xTaskHandle xHandle;
    DisplacementMotor_MoveHandler moveHandler;
    MoveResult moveResult;
}DisplacementMotor;

void DisplacementMotor_Init();
void DisplacementMotor_Restore();
StepperMotorParam DisplacementMotor_GetDefaultMotionParam(Uint8 index);
StepperMotorParam DisplacementMotor_GetCurrentMotionParam(Uint8 index);
Uint16 DisplacementMotor_SetDefaultMotionParam(Uint8 index, StepperMotorParam param);
Uint16 DisplacementMotor_SetCurrentMotionParam(Uint8 index, StepperMotorParam param);
DisplacementMotorStatus DisplacementMotor_GetStatus(Uint8 index);
Uint32 DisplacementMotor_GetMaxSteps(Uint8 index);
Uint32 DisplacementMotor_GetCurrentSteps(Uint8 index);
Bool DisplacementMotor_IsSensorBlocked(Uint8 index);
Bool DisplacementMotor_CheckCollision(Uint8 index);
Bool DisplacementMotor_SendEventOpen(Uint8 index);
MoveResult DisplacementMotor_GetMoveResult(Uint8 index);
char* DisplacementMotor_GetName(Uint8 index);
Uint16 DisplacementMotor_Start(Uint8 index, Int16 step, DisplacementMotorMode mode, Bool isUseDefaultParam);
Uint16 DisplacementMotor_Reset(Uint8 index);
Uint16 DisplacementMotor_RequestStop(Uint8 index);
StepperMotor* DisplacementMotor_GetStepperMotorX(void);
StepperMotor* DisplacementMotor_GetStepperMotorZ(void);

#endif /* SRC_PERISTALTICPUMP_DISPLACEMENTMOTORMANAGER_H_ */
