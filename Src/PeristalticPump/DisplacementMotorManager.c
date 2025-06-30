/*
 * DisplacementMotorManager.c
 *
 *  Created on: 2018年3月7日
 *      Author: LIANG
 */
#include "DNCP/App/DscpSysDefine.h"
#include "System.h"
#include "Driver/LiquidDriver/PositionSensorMap.h"
#include "Driver/LiquidDriver/StepperMotorMap.h"
#include "StepperMotor.h"
#include "SystemConfig.h"
#include "Driver/McuFlash.h"
#include "DncpStack/DncpStack.h"
#include "DisplacementMotorManager.h"
#include <stdlib.h>
#include "LuipApi/MotorControlInterface.h"

DisplacementMotor g_displacementMotors[DISPLACEMENTMOTOR_TOTAL_PUMP];
PositionSensor g_collisionSensors[2];

#define LIMITSTEPMAX 4
#define MAXOFFLIMITSTEP 10
#define SUBDIVISION_X 4
#define SUBDIVISION_Z 16

/*********Task*********/
static void DisplacementMotor_TaskHandle(void *pvParameters);
/*********Function*********/
static void DisplacementMotor_ResetHandle(void* obj);
static void DisplacementMotor_MoveToTargetLocationHandle(void* obj);
static void DisplacementMotor_MoveToSensorOffLimitHandle(void *obj);
static void DisplacementMotor_ZForWardMoveOffLimitHandle(void *obj);
static void DisplacementMotor_XForWardMoveOffLimitHandle(void *obj);
static void DisplacementMotor_XMoveToZerosOffLimitHandle(void *obj);
static void DisplacementMotor_ZMoveToZerosOffLimitHandle(void *obj);
static Bool DisplacementMotor_XMoveCheckPosition(DisplacementMotor *displacementMotor, Direction dir);
static Bool DisplacementMotor_ZMoveCheckPosition(DisplacementMotor *displacementMotor, Direction dir);
static void DisplacementMotor_SendEvent(DisplacementMotor *displacementMotor, MoveResult moveResult);
static void DisplacementMotor_Stop(DisplacementMotor *displacementMotor, MoveResult moveResult);
/*********Variable*********/
static StepperMotorParam s_resetXParam = {800, 800};
static StepperMotorParam s_resetZParam = {400, 400};

void DisplacementMotor_Init()
{
    memset(g_displacementMotors, 0, sizeof(DisplacementMotor) * DISPLACEMENTMOTOR_TOTAL_PUMP);
    memset(g_collisionSensors, 0, sizeof(PositionSensor) * 2);

    PositionSensorMap_DisplacementMotorInit(g_displacementMotors);
    PositionSensorMap_CollisionSensorInit(g_collisionSensors);
    StepperMotorMap_DisplacementMotorInit(g_displacementMotors);

    for (Uint8 i = 0; i < DISPLACEMENTMOTOR_TOTAL_PUMP; i++)
    {
        if(i == X_DISPLACEMENTMOTOR)
        {
            StepperMotor_SetSubdivision(&g_displacementMotors[i].stepperMotor, SUBDIVISION_X);
        }
        else if(i == Z_DISPLACEMENTMOTOR)
        {
            StepperMotor_SetSubdivision(&g_displacementMotors[i].stepperMotor, SUBDIVISION_Z);
        }

        Uint8 buffer[DISPLACEMENTMOTOR_SIGN_FLASH_LEN] = { 0 };
        Uint32 flashFactorySign = 0;
        StepperMotorParam param;

        McuFlash_Read(DISPLACEMENTMOTOR_SIGN_FLASH_BASE_ADDR + DISPLACEMENTMOTOR_SIGN_FLASH_LEN * i,
                DISPLACEMENTMOTOR_SIGN_FLASH_LEN, buffer); //读取出厂标志位
        memcpy(&flashFactorySign, buffer, DISPLACEMENTMOTOR_SIGN_FLASH_LEN);

        if (FLASH_FACTORY_SIGN == flashFactorySign) //表示已经过出厂设置
        {
            param = DisplacementMotor_GetDefaultMotionParam(i);
        }
        else
        {
            param.acceleration = 400;
            param.maxSpeed = 400;
            DisplacementMotor_SetDefaultMotionParam(i, param);

            flashFactorySign = FLASH_FACTORY_SIGN;
            memcpy(buffer, &flashFactorySign, DISPLACEMENTMOTOR_SIGN_FLASH_LEN);
            McuFlash_Write(
                    DISPLACEMENTMOTOR_SIGN_FLASH_BASE_ADDR + DISPLACEMENTMOTOR_SIGN_FLASH_LEN * i,
                    DISPLACEMENTMOTOR_SIGN_FLASH_LEN, buffer);
        }
        StepperMotor_SetNumber(&g_displacementMotors[i].stepperMotor, i);
        StepperMotor_Init(&g_displacementMotors[i].stepperMotor, param);
        StepperMotor_SetPosLockStatus(&g_displacementMotors[i].stepperMotor, TRUE);

        g_displacementMotors[i].currentSteps = 0;
        g_displacementMotors[i].isRequestStop = FALSE;
        g_displacementMotors[i].isSendEvent = FALSE;
        g_displacementMotors[i].isStatusSwitchStart = FALSE;
        g_displacementMotors[i].limitStepCount = 0;
        g_displacementMotors[i].moveHandler = NULL;
        g_displacementMotors[i].status = MOTOR_IDLE;
        g_displacementMotors[i].returnStepCount = 0;
        g_displacementMotors[i].isChangeSteps = FALSE;
    }
    g_displacementMotors[X_DISPLACEMENTMOTOR].maxSteps = 1050;
    g_displacementMotors[Z_DISPLACEMENTMOTOR].maxSteps = 400;

    xTaskCreate(DisplacementMotor_TaskHandle, "XDisplacementMotorHandle", DISPLACEMENTMOTOR_STK_SIZE, (void *)&g_displacementMotors[X_DISPLACEMENTMOTOR],
            DISPLACEMENTMOTOR_TASK_PRIO, &g_displacementMotors[X_DISPLACEMENTMOTOR].xHandle);

    xTaskCreate(DisplacementMotor_TaskHandle, "ZDisplacementMotorHandle", DISPLACEMENTMOTOR_STK_SIZE, (void *)&g_displacementMotors[Z_DISPLACEMENTMOTOR],
            DISPLACEMENTMOTOR_TASK_PRIO, &g_displacementMotors[Z_DISPLACEMENTMOTOR].xHandle);
}

void DisplacementMotor_Restore()
{
    for (Uint8 i = 0; i < DISPLACEMENTMOTOR_TOTAL_PUMP; i++)
    {
        g_displacementMotors[i].isSendEvent = FALSE;
        DisplacementMotor_RequestStop(i);
    }
}

StepperMotor* DisplacementMotor_GetStepperMotorX(void)
{
    return &g_displacementMotors[X_DISPLACEMENTMOTOR].stepperMotor;
}

StepperMotor* DisplacementMotor_GetStepperMotorZ(void)
{
    return &g_displacementMotors[Z_DISPLACEMENTMOTOR].stepperMotor;
}

StepperMotorParam DisplacementMotor_GetDefaultMotionParam(Uint8 index)
{
    StepperMotorParam param;
    memset(&param, 0, sizeof(StepperMotorParam));

    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        Uint8 readData[DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_LEN] ={ 0 };

        McuFlash_Read(DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_BASE_ADDR + DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_LEN * index,
                DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_LEN, readData);

        memcpy(&param, readData, sizeof(StepperMotorParam));
        return param;
    }
    else
    {
        TRACE_ERROR("\n No. %d  DisplacementMotor.", index);
        return param;
    }
}

StepperMotorParam DisplacementMotor_GetCurrentMotionParam(Uint8 index)
{
    StepperMotorParam param;
    memset(&param, 0, sizeof(StepperMotorParam));

    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        param = StepperMotor_GetCurrentMotionParam(&g_displacementMotors[index].stepperMotor);
        return param;
    }
    else
    {
        TRACE_ERROR("\n No. %d DisplacementMotor.", index);
        return param;
    }
}

Uint16 DisplacementMotor_SetDefaultMotionParam(Uint8 index, StepperMotorParam param)
{
    float lowSpeed = STEPPERMOTOR_MIN_SUBDIVISION_SPEED  / StepperMotor_GetSubdivision(&g_displacementMotors[index].stepperMotor);
    float highSpeed = STEPPERMOTOR_MAX_SUBDIVISION_SPEED / StepperMotor_GetSubdivision(&g_displacementMotors[index].stepperMotor);

    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP && 0 != param.acceleration
            && (param.maxSpeed >= lowSpeed)
            && (param.maxSpeed <= highSpeed))
    {
        TRACE_INFO("\n %s set default param acc :", DisplacementMotor_GetName(index));
        System_PrintfFloat(TRACE_LEVEL_INFO, param.acceleration, 4);
        TRACE_INFO(" step/(s^2),maxSpeed:");
        System_PrintfFloat(TRACE_LEVEL_INFO, param.maxSpeed, 4);
        TRACE_INFO(" step/s");

        if (TRUE == StepperMotor_SetDefaultMotionParam(&g_displacementMotors[index].stepperMotor, param))
        {
            Uint8 writeData[DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_LEN] ={ 0 };
            memcpy(writeData, &param, sizeof(StepperMotorParam));
            McuFlash_Write(
                    DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_BASE_ADDR + DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_LEN * index,
                    DISPLACEMENTMOTOR_MOTIONPARAM_FLASH_LEN, writeData);
            return DSCP_OK;
        }
        else
        {
            return DSCP_ERROR;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n The DisplacementMotor %d motion default parameter setting failed because of the parameter error.", index);

        TRACE_ERROR("\n DisplacementMotor %d set default param maxSpeed :", index);
        System_PrintfFloat(TRACE_LEVEL_ERROR, param.maxSpeed, 4);
        TRACE_ERROR(" step/s,acc:");
        System_PrintfFloat(TRACE_LEVEL_ERROR, param.acceleration, 4);
        TRACE_ERROR(" step/(s^2)\n lowSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, lowSpeed, 4);
        TRACE_ERROR(" step/s\n highSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, highSpeed, 4);
        TRACE_ERROR(" step/s\n");

        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

Uint16 DisplacementMotor_SetCurrentMotionParam(Uint8 index, StepperMotorParam param)
{
    float lowSpeed = STEPPERMOTOR_MIN_SUBDIVISION_SPEED  / StepperMotor_GetSubdivision(&g_displacementMotors[index].stepperMotor);
       float highSpeed = STEPPERMOTOR_MAX_SUBDIVISION_SPEED / StepperMotor_GetSubdivision(&g_displacementMotors[index].stepperMotor);

       if (index < DISPLACEMENTMOTOR_TOTAL_PUMP && 0 != param.acceleration
               && (param.maxSpeed >= lowSpeed)
               && (param.maxSpeed <= highSpeed))
       {
           TRACE_INFO("\n %s set current param maxSpeed:",  DisplacementMotor_GetName(index));
           System_PrintfFloat(TRACE_LEVEL_INFO, param.maxSpeed, 4);
           TRACE_INFO(" step/s,acc:");
           System_PrintfFloat(TRACE_LEVEL_INFO, param.acceleration, 4);
           TRACE_INFO(" step/(s^2) ");

           StepperMotor_SetCurrentMotionParam(&g_displacementMotors[index].stepperMotor, param);
           return DSCP_OK;
       }
       else
       {
           TRACE_ERROR(
                   "\n The DisplacementMotor %d motion current parameter setting failed because of the parameter error.", index);

           TRACE_ERROR("\n DisplacementMotor %d set current param maxSpeed :", index);
           System_PrintfFloat(TRACE_LEVEL_ERROR, param.maxSpeed, 4);
           TRACE_ERROR(" step/s,acc:");
           System_PrintfFloat(TRACE_LEVEL_ERROR, param.acceleration, 4);
           TRACE_ERROR(" step/(s^2)\n lowSpeed :");
           System_PrintfFloat(TRACE_LEVEL_ERROR, lowSpeed, 4);
           TRACE_ERROR(" step/s\n highSpeed :");
           System_PrintfFloat(TRACE_LEVEL_ERROR, highSpeed, 4);
           TRACE_ERROR(" step/s\n");

           return ((Uint16) DSCP_ERROR_PARAM);
       }
}

DisplacementMotorStatus DisplacementMotor_GetStatus(Uint8 index)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        return g_displacementMotors[index].status;
    }
    else
    {
        TRACE_ERROR("\n No. %d DisplacementMotor.", index);
        return MOTOR_IDLE;
    }
}

Uint32 DisplacementMotor_GetMaxSteps(Uint8 index)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        return g_displacementMotors[index].maxSteps;
    }
    else
    {
        TRACE_ERROR("\n No. %d DisplacementMotor.", index);
        return 0;
    }
}

Uint32 DisplacementMotor_GetCurrentSteps(Uint8 index)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        return g_displacementMotors[index].currentSteps;
    }
    else
    {
        TRACE_ERROR("\n No. %d DisplacementMotor.", index);
        return 0;
    }
}

Bool DisplacementMotor_IsSensorBlocked(Uint8 index)
{
    Bool ret;
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        if (SENSOR_HIGH_LEVEL == PositionSensor_ReadInputStatus(&g_displacementMotors[index].positionSensor))
        {
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n No. %d DisplacementMotor.", index);
        ret = FALSE;
    }
    return ret;
}

Bool DisplacementMotor_CheckCollision(Uint8 index)
{
    Bool ret;
    if (index < 2)
    {
        if (SENSOR_LOW_LEVEL == PositionSensor_ReadInputStatus(&g_collisionSensors[index]))
        {
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n Invalid No. %d Collision sensor.", index);
        ret = FALSE;
    }
    return ret;
}

Bool DisplacementMotor_SendEventOpen(Uint8 index)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        g_displacementMotors[index].isSendEvent = TRUE;
        return TRUE;
    }
    else
    {
        TRACE_ERROR("\n Invalid No. %d DisplacementMotor.", index);
        return FALSE;
    }
}

MoveResult DisplacementMotor_GetMoveResult(Uint8 index)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        return g_displacementMotors[index].moveResult;;
    }
    else
    {
        return RESULT_FAILED;
    }
}

char* DisplacementMotor_GetName(Uint8 index)
{
    static char name[20] = "";
    memset(name, 0, sizeof(name));
    switch(index)
    {
    case X_DISPLACEMENTMOTOR:
        strcpy(name, "XDisplacementMotor");
        break;
    case Z_DISPLACEMENTMOTOR:
        strcpy(name, "ZDisplacementMotor");
        break;
    default:
        strcpy(name, "NULL");
        break;
    }
    return name;
}

void DisplacementMotor_TaskHandle(void *pvParameters)
{
    DisplacementMotor *displacementMotor = (DisplacementMotor *)pvParameters;
    vTaskSuspend(NULL);
    while(1)
    {
        vTaskDelay(5 / portTICK_RATE_MS);
        switch (displacementMotor->status)
        {
        case MOTOR_IDLE:
            vTaskSuspend(NULL);
            break;
        default:
            if (displacementMotor->moveHandler)
            {
                displacementMotor->moveHandler(displacementMotor);
            }
            break;
        }
    }
}

void DisplacementMotor_MoveToSensorOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    Uint8 number = StepperMotor_GetNumber(stepperMotor);

    if(FALSE == g_displacementMotors[number].isChangeSteps)
    {
        g_displacementMotors[number].returnStepCount--;
        if(g_displacementMotors[number].returnStepCount < 0)
        {
            TRACE_INFO("\nMotor %d Change Step = %d", number, g_displacementMotors[number].maxSteps + MAXOFFLIMITSTEP);
            StepperMotor_ChangeStep(stepperMotor, g_displacementMotors[number].maxSteps + MAXOFFLIMITSTEP);
            g_displacementMotors[number].isChangeSteps = TRUE;
            g_displacementMotors[number].returnStepCount = 0;
        }
    }

    // 如果遮住传感器
    if (TRUE == DisplacementMotor_IsSensorBlocked(number))
    {
        g_displacementMotors[number].limitStepCount++;
        if (g_displacementMotors[number].limitStepCount >= LIMITSTEPMAX)
        {
        //停止运行
            StepperMotor_ImmediatelyStop(stepperMotor);
            TRACE_INFO("\n %s move to sensor.PositionSensor status %d", DisplacementMotor_GetName(number), PositionSensor_ReadInputStatus(&g_displacementMotors[number].positionSensor));
        }
    }
    else
    {
        g_displacementMotors[number].limitStepCount = 0;
    }
    if(number == X_DISPLACEMENTMOTOR)  //X轴移向传感器检测撞针
    {
        if (TRUE == DisplacementMotor_CheckCollision(0)
                    || TRUE == DisplacementMotor_CheckCollision(1))
        {
            StepperMotor_ImmediatelyStop(stepperMotor);
            TRACE_ERROR("\n Collision is Collided. 0 Collision status %d.1 Collision status %d",
                    PositionSensor_ReadInputStatus(&g_collisionSensors[0]),
                    PositionSensor_ReadInputStatus(&g_collisionSensors[1]));
        }
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

void DisplacementMotor_ZForWardMoveOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    if (TRUE == DisplacementMotor_CheckCollision(0))
    {
        StepperMotor_ImmediatelyStop(stepperMotor);
        TRACE_ERROR("\n 0 Collision is Collided.Collision status %d", PositionSensor_ReadInputStatus(&g_collisionSensors[0]));
    }
    else if (TRUE == DisplacementMotor_CheckCollision(1))
    {
        StepperMotor_ImmediatelyStop(stepperMotor);
        TRACE_ERROR("\n 1 Collision is Collided.Collision status %d", PositionSensor_ReadInputStatus(&g_collisionSensors[1]));
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

void DisplacementMotor_XForWardMoveOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    if (TRUE == DisplacementMotor_CheckCollision(0))
    {
        StepperMotor_ImmediatelyStop(stepperMotor);
        TRACE_ERROR("\n 0 Collision is Collided.Collision status %d", PositionSensor_ReadInputStatus(&g_collisionSensors[0]));
    }
    else if (TRUE == DisplacementMotor_CheckCollision(1))
    {
        StepperMotor_ImmediatelyStop(stepperMotor);
        TRACE_ERROR("\n 1 Collision is Collided.Collision status %d", PositionSensor_ReadInputStatus(&g_collisionSensors[1]));
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

void DisplacementMotor_XMoveToZerosOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    // 如果没遮住传感器
    if (FALSE == DisplacementMotor_IsSensorBlocked(X_DISPLACEMENTMOTOR))
    {
        g_displacementMotors[X_DISPLACEMENTMOTOR].limitStepCount++;
        if (g_displacementMotors[X_DISPLACEMENTMOTOR].limitStepCount >= LIMITSTEPMAX)
        {
            //停止运行
            StepperMotor_ImmediatelyStop(stepperMotor);
            TRACE_INFO("\n %s move to zeros.PositionSensor status %d", DisplacementMotor_GetName(X_DISPLACEMENTMOTOR)
                    , PositionSensor_ReadInputStatus(&g_displacementMotors[X_DISPLACEMENTMOTOR].positionSensor));
        }
    }
    else
    {
        g_displacementMotors[X_DISPLACEMENTMOTOR].limitStepCount = 0;
    }
    if (TRUE == DisplacementMotor_CheckCollision(0)
                || TRUE == DisplacementMotor_CheckCollision(1))
    {
        StepperMotor_ImmediatelyStop(stepperMotor);
        TRACE_ERROR("\n Collision is Collided. 0 Collision status %d.1 Collision status %d",
                PositionSensor_ReadInputStatus(&g_collisionSensors[0]),
                PositionSensor_ReadInputStatus(&g_collisionSensors[1]));
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

void DisplacementMotor_ZMoveToZerosOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    // 如果没遮住传感器
    if (FALSE == DisplacementMotor_IsSensorBlocked(Z_DISPLACEMENTMOTOR))
    {
        g_displacementMotors[Z_DISPLACEMENTMOTOR].limitStepCount++;
        if (g_displacementMotors[Z_DISPLACEMENTMOTOR].limitStepCount >= LIMITSTEPMAX)
        {
            //停止运行
            StepperMotor_ImmediatelyStop(stepperMotor);
            TRACE_INFO("\n %s move to zeros.PositionSensor status %d",  DisplacementMotor_GetName(Z_DISPLACEMENTMOTOR),
                    PositionSensor_ReadInputStatus(&g_displacementMotors[Z_DISPLACEMENTMOTOR].positionSensor));
        }
    }
    else
    {
        g_displacementMotors[Z_DISPLACEMENTMOTOR].limitStepCount = 0;
    }
    if (TRUE == DisplacementMotor_CheckCollision(0)
                || TRUE == DisplacementMotor_CheckCollision(1))
    {
        StepperMotor_ImmediatelyStop(stepperMotor);
        TRACE_ERROR("\n Collision is Collided. 0 Collision status %d.1 Collision status %d",
                PositionSensor_ReadInputStatus(&g_collisionSensors[0]),
                PositionSensor_ReadInputStatus(&g_collisionSensors[1]));
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

Bool DisplacementMotor_XMoveCheckPosition(DisplacementMotor *displacementMotor, Direction dir)
{
    Bool ret = TRUE;
    if (BACKWARD == dir && TRUE == DisplacementMotor_IsSensorBlocked(X_DISPLACEMENTMOTOR))//已在零点，不能向后退
    {
        ret = FALSE;
        TRACE_ERROR("\n X displacementMotor is at the starting point and cannot fall back");
    }
    else if (TRUE == DisplacementMotor_CheckCollision(0)
            || TRUE == DisplacementMotor_CheckCollision(1))//撞针
    {
        ret = FALSE;
        TRACE_ERROR("\n Collision is Blocked.");
    }
    return ret;
}

Bool DisplacementMotor_ZMoveCheckPosition(DisplacementMotor *displacementMotor, Direction dir)
{
    Bool ret = TRUE;
    if (BACKWARD == dir && TRUE == DisplacementMotor_IsSensorBlocked(Z_DISPLACEMENTMOTOR))//已在零点，不能向后退
    {
        ret = FALSE;
        TRACE_ERROR("\n Z displacementMotor is at the starting point and cannot fall back");
    }
    else if (FORWARD == dir
            && (TRUE == DisplacementMotor_CheckCollision(0)
            || TRUE == DisplacementMotor_CheckCollision(1)))//撞针，不能向前
    {
        ret = FALSE;
        TRACE_ERROR("\n Collision is Blocked.");
    }
    return ret;
}

Uint16 DisplacementMotor_Start(Uint8 index, Int16 step, DisplacementMotorMode mode, Bool isUseDefaultParam)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP
            && abs(step) <=  g_displacementMotors[index].maxSteps
            && mode < MAX_MOTOR_MOVE_MODE)
    {
        if (MOTOR_IDLE == g_displacementMotors[index].status)
        {

            if (MOTOR_MOVE_ABSOLUTE_MODE == mode)
            {
                 if (step >= 0 && step != g_displacementMotors[index].currentSteps)
                 {
                     if (step > g_displacementMotors[index].currentSteps)
                     {
                         g_displacementMotors[index].dir = FORWARD;
                         g_displacementMotors[index].targetStep = step - g_displacementMotors[index].currentSteps;
                     }
                     else
                     {
                         g_displacementMotors[index].dir = BACKWARD;
                         g_displacementMotors[index].targetStep = g_displacementMotors[index].currentSteps - step;
                     }
                 }
                 else
                 {
                     TRACE_ERROR(
                             "\n Because of the parameter error(mode = MOTOR_MOVE_ABSOLUTE_MODE , step < 0 or step = currentSteps), the %s starts to fail. ", DisplacementMotor_GetName(index));
                     return ((Uint16) DSCP_ERROR_PARAM);
                 }
            }
            else
            {
                if (step != 0)
                {
                    if (step > 0)
                    {
                        g_displacementMotors[index].dir = FORWARD;
                        g_displacementMotors[index].targetStep = step;
                    }
                    else
                    {
                        g_displacementMotors[index].dir = BACKWARD;
                        g_displacementMotors[index].targetStep = -1 * step;
                    }

                }
                else
                {
                    TRACE_ERROR(
                            "\n Because of the parameter error(mode = MOTOR_MOVE_RELATIVE_MODE or MOTOR_MOVE_SAFE_MODE , step = 0), the %s starts to fail.", DisplacementMotor_GetName(index));
                    return ((Uint16) DSCP_ERROR_PARAM);
                }
            }

            DisplacementMotorStatus status = MOTOR_TO_TARGET_LOCATION;
            if (index == X_DISPLACEMENTMOTOR && MOTOR_MOVE_SAFE_MODE == mode)
            {
                status = MOTOR_TO_WAIT_Z_RESET;
                if (MOTOR_IDLE != g_displacementMotors[Z_DISPLACEMENTMOTOR].status)
                {
                    TRACE_ERROR("\n Because the Z DisplacementMotor is running, the %s safe mode starts to fail.", DisplacementMotor_GetName(index));
                    return DSCP_BUSY;
                }
            }

            if (index == Z_DISPLACEMENTMOTOR)
            {
                if (FALSE == DisplacementMotor_ZMoveCheckPosition(&g_displacementMotors[Z_DISPLACEMENTMOTOR], g_displacementMotors[Z_DISPLACEMENTMOTOR].dir))
                {
                    TRACE_ERROR("\n Because Z DisplacementMotor position error, the Z DisplacementMotor start fail.");
                    return ((Uint16) DSCP_ERROR);
                }
            }
            else
            {
                if (FALSE == DisplacementMotor_XMoveCheckPosition(&g_displacementMotors[X_DISPLACEMENTMOTOR], g_displacementMotors[X_DISPLACEMENTMOTOR].dir))
                {
                    TRACE_ERROR("\n Because X DisplacementMotor position error, the X DisplacementMotor start fail.");
                    return ((Uint16) DSCP_ERROR);
                }
            }

            TRACE_INFO("\n %s start mode:%d, step:%d", DisplacementMotor_GetName(index), mode, step);
            TRACE_INFO("\n dir: %d, targetStep:%d, currentSteps: %d", g_displacementMotors[index].dir, g_displacementMotors[index].targetStep, g_displacementMotors[index].currentSteps);

            g_displacementMotors[index].mode = mode;
            g_displacementMotors[index].isUseDefaultParam = isUseDefaultParam;

            g_displacementMotors[index].isRequestStop = FALSE;
            g_displacementMotors[index].limitStepCount = 0;

            g_displacementMotors[index].moveHandler = DisplacementMotor_MoveToTargetLocationHandle;
            g_displacementMotors[index].status = status;
            g_displacementMotors[index].isStatusSwitchStart = TRUE;
            DncpStack_ClearBufferedEvent();
            vTaskResume(g_displacementMotors[index].xHandle);
            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR("\n Because the DisplacementMotor is running, the %s starts to fail.", DisplacementMotor_GetName(index));
            return DSCP_BUSY;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n Because of the parameter error, the DisplacementMotor %d starts to fail. ", index);
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

Uint16 DisplacementMotor_Reset(Uint8 index)
{

    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        if (MOTOR_IDLE == g_displacementMotors[index].status)
        {
            TRACE_INFO("\n %s start reset", DisplacementMotor_GetName(index));
            g_displacementMotors[index].isRequestStop = FALSE;
            g_displacementMotors[index].limitStepCount = 0;

            g_displacementMotors[index].moveHandler = DisplacementMotor_ResetHandle;
            g_displacementMotors[index].status = MOTOR_TO_SENSOR;
            g_displacementMotors[index].isStatusSwitchStart = TRUE;
            DncpStack_ClearBufferedEvent();
            vTaskResume(g_displacementMotors[index].xHandle);
            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR("\n Because the DisplacementMotor is running, the %s reset to fail.", DisplacementMotor_GetName(index));
            return DSCP_BUSY;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n Because of the parameter error, the DisplacementMotor %d reset to fail. ", index);
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

Uint16 DisplacementMotor_RequestStop(Uint8 index)
{
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        if(MOTOR_IDLE !=  g_displacementMotors[index].status)
        {
            if (index == X_DISPLACEMENTMOTOR
                    && MOTOR_MOVE_SAFE_MODE == g_displacementMotors[X_DISPLACEMENTMOTOR].mode
                    && MOTOR_IDLE != DisplacementMotor_GetStatus(Z_DISPLACEMENTMOTOR))
            {
                DisplacementMotor_RequestStop(Z_DISPLACEMENTMOTOR);
            }
            g_displacementMotors[index].isRequestStop = TRUE;
            StepperMotor_RequestStop(&g_displacementMotors[index].stepperMotor);
            TRACE_INFO("\n %s RequestStop", DisplacementMotor_GetName(index));
            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR("\n %s does not run, stop failure.", DisplacementMotor_GetName(index));
            return DSCP_ERROR;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n Parameter error, the DisplacementMotor %d stop to fail. ", index);
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

void DisplacementMotor_SendEvent(DisplacementMotor *displacementMotor, MoveResult moveResult)
{
    if(TRUE == displacementMotor->isSendEvent)
    {
        Uint8 data[10] = {0};
        data[0] =  StepperMotor_GetNumber(&displacementMotor->stepperMotor);
        memcpy(data + sizeof(Uint8), &moveResult, sizeof(moveResult));
        DncpStack_SendEvent(DSCP_EVENT_MCI_MOTOR_RESULT, data , sizeof(Uint8) + sizeof(moveResult));
        DncpStack_BufferEvent(DSCP_EVENT_MCI_MOTOR_RESULT, data , sizeof(Uint8) + sizeof(moveResult));
    }
    displacementMotor->isSendEvent = FALSE;
}

void DisplacementMotor_Stop(DisplacementMotor *displacementMotor, MoveResult moveResult)
{
    if((RESULT_FINISHED != moveResult) && (RESULT_STOPPED != moveResult))
    {
        StepperMotor_DriverCheck(&displacementMotor->stepperMotor);
    }
    DisplacementMotor_SendEvent(displacementMotor, moveResult);
    displacementMotor->status = MOTOR_IDLE;
    displacementMotor->isStatusSwitchStart = TRUE;
    displacementMotor->moveResult = moveResult;
    TRACE_INFO("\n DisplacementMotor Stop. result = %d", (Uint8)moveResult);
}

void DisplacementMotor_MoveToTargetLocationHandle(void* obj)
{
    DisplacementMotor *displacementMotor = (DisplacementMotor *)obj;
    if (MOTOR_TO_WAIT_Z_RESET == displacementMotor->status)
    {
        if (TRUE == displacementMotor->isStatusSwitchStart)
        {
            displacementMotor->isStatusSwitchStart = FALSE;
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);
            TRACE_DEBUG("\n %s status: MOTOR_TO_WAIT_Z_RESET", DisplacementMotor_GetName(number));
            if (DSCP_OK != DisplacementMotor_Reset(Z_DISPLACEMENTMOTOR))
            {
                TRACE_ERROR("\n Because Z DisplacementMotor start reset fail, the %s fail.", DisplacementMotor_GetName(number));
                DisplacementMotor_Stop(displacementMotor, RESULT_FAILED);
                return;
            }
        }
        if (MOTOR_IDLE == DisplacementMotor_GetStatus(Z_DISPLACEMENTMOTOR))
        {
            MoveResult moveResult = DisplacementMotor_GetMoveResult(Z_DISPLACEMENTMOTOR);
            if (RESULT_FINISHED == moveResult)
            {
                displacementMotor->status = MOTOR_TO_TARGET_LOCATION;
                displacementMotor->isStatusSwitchStart = TRUE;
            }
            else
            {
                TRACE_ERROR("\n Because Z DisplacementMotor reset %d, the %s fail.", moveResult, DisplacementMotor_GetName(StepperMotor_GetNumber(&displacementMotor->stepperMotor)));
                DisplacementMotor_Stop(displacementMotor, moveResult);
            }
        }
    }
    else if (MOTOR_TO_TARGET_LOCATION == displacementMotor->status)
    {
        if (TRUE == displacementMotor->isStatusSwitchStart)
        {
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);
            TRACE_DEBUG("\n %s status: MOTOR_TO_TARGET_LOCATION", DisplacementMotor_GetName(number));
            displacementMotor->isStatusSwitchStart = FALSE;
            StepperMotor_OtherMoveHandler limitHandle = NULL;
            if (displacementMotor->dir == BACKWARD)//向零点运动则检查限位传感器
            {
                displacementMotor->returnStepCount = displacementMotor->currentSteps + LIMITSTEPMAX + 1;
                displacementMotor->isChangeSteps = TRUE;
                limitHandle = DisplacementMotor_MoveToSensorOffLimitHandle;
            }
            else if (displacementMotor->dir == FORWARD && Z_DISPLACEMENTMOTOR == number)//z轴向下运动检查撞针
            {
                limitHandle = DisplacementMotor_ZForWardMoveOffLimitHandle;
            }
            else if (displacementMotor->dir == FORWARD && X_DISPLACEMENTMOTOR == number)//x轴前移运动检查撞针
            {
                limitHandle = DisplacementMotor_XForWardMoveOffLimitHandle;
            }

            if  (FALSE == StepperMotor_Start(&displacementMotor->stepperMotor, displacementMotor->dir, displacementMotor->targetStep, displacementMotor->isUseDefaultParam, limitHandle))
            {
                TRACE_ERROR("\n Because no idle timer , the %s fail.", DisplacementMotor_GetName(number));
                DisplacementMotor_Stop(displacementMotor, RESULT_FAILED);
                return;
            }
        }

        if (StepperMotor_IDLE == StepperMotor_GetStatus(&displacementMotor->stepperMotor))
        {
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);
            MoveResult moveResult;
            if (TRUE == displacementMotor->isRequestStop)
            {
                moveResult = RESULT_STOPPED;
            }
            else if (number == Z_DISPLACEMENTMOTOR && FALSE == DisplacementMotor_ZMoveCheckPosition(&g_displacementMotors[Z_DISPLACEMENTMOTOR], g_displacementMotors[Z_DISPLACEMENTMOTOR].dir))
            {
                if(TRUE == DisplacementMotor_CheckCollision(0))
                {
                    TRACE_ERROR("\n Collision check sensor 0 is Collided.");
                    moveResult = RESULT_COLLISION_0;
                }
                else if(TRUE == DisplacementMotor_CheckCollision(1))
                {
                    TRACE_ERROR("\n Collision check sensor 1 is Collided");
                    moveResult = RESULT_COLLISION_1;
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&displacementMotor->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    moveResult = RESULT_DRIVER_ERROR;
                }
                else
                {
                    TRACE_ERROR("\n Because Z DisplacementMotor position error, the Z DisplacementMotor fail.");
                    moveResult = RESULT_FAILED;
                }
            }
            else if (number == X_DISPLACEMENTMOTOR && FALSE == DisplacementMotor_XMoveCheckPosition(&g_displacementMotors[X_DISPLACEMENTMOTOR], g_displacementMotors[X_DISPLACEMENTMOTOR].dir))
            {
                if(TRUE == DisplacementMotor_CheckCollision(0))
                {
                    TRACE_ERROR("\n Collision check sensor 0 is Collided.");
                    moveResult = RESULT_COLLISION_0;
                }
                else if(TRUE == DisplacementMotor_CheckCollision(1))
                {
                    TRACE_ERROR("\n Collision check sensor 1 is Collided");
                    moveResult = RESULT_COLLISION_1;
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&displacementMotor->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    moveResult = RESULT_DRIVER_ERROR;
                }
                else
                {
                    TRACE_ERROR("\n Because X DisplacementMotor position error, the X DisplacementMotor fail.");
                    moveResult = RESULT_FAILED;
                }
            }
            else
            {
                moveResult = RESULT_FINISHED;
            }

            Uint32 alreadyStep = StepperMotor_GetAlreadyStep(&displacementMotor->stepperMotor);
            if (displacementMotor->dir == BACKWARD)
            {
                displacementMotor->currentSteps -= alreadyStep;
                if (displacementMotor->currentSteps < 0)//在相对模式中，可能往后退的步数大于当前步数，为能复位回到零点允许此操作
                {
                    displacementMotor->currentSteps = 0;
                }
                TRACE_INFO("\n %s backward Step : %d new currentSteps: %d", DisplacementMotor_GetName(number), alreadyStep, displacementMotor->currentSteps);
            }
            else
            {
                displacementMotor->currentSteps += alreadyStep;
                TRACE_INFO("\n %s forward Step : %d new currentSteps: %d", DisplacementMotor_GetName(number), alreadyStep, displacementMotor->currentSteps);
            }
            DisplacementMotor_Stop(displacementMotor, moveResult);
        }
    }
}

void DisplacementMotor_ResetHandle(void* obj)
{
    DisplacementMotor *displacementMotor = (DisplacementMotor *)obj;

    //使用复位速度
    if(displacementMotor->stepperMotor.number == X_DISPLACEMENTMOTOR)
    {
        StepperMotor_SetCurrentMotionParam(&displacementMotor->stepperMotor, s_resetXParam);
    }
    else if(displacementMotor->stepperMotor.number == Z_DISPLACEMENTMOTOR)
    {
        StepperMotor_SetCurrentMotionParam(&displacementMotor->stepperMotor, s_resetZParam);
    }

    if (MOTOR_TO_SENSOR == displacementMotor->status)
    {
        if (TRUE == displacementMotor->isStatusSwitchStart)
        {
            displacementMotor->isStatusSwitchStart = FALSE;
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);
            TRACE_DEBUG("\n %s status: MOTOR_TO_SENSOR", DisplacementMotor_GetName(number));

            displacementMotor->returnStepCount = displacementMotor->currentSteps + LIMITSTEPMAX + 1;
            if(displacementMotor->returnStepCount > displacementMotor->maxSteps + MAXOFFLIMITSTEP)
            {
                displacementMotor->returnStepCount = displacementMotor->maxSteps + MAXOFFLIMITSTEP;
            }
            displacementMotor->isChangeSteps = FALSE;
            TRACE_INFO("\nMotor %d returnStepCount = %d", number, displacementMotor->returnStepCount);
            ///if  (FALSE == StepperMotor_Start(&displacementMotor->stepperMotor, BACKWARD, displacementMotor->maxSteps + MAXOFFLIMITSTEP, FALSE, DisplacementMotor_MoveToSensorOffLimitHandle))
            if  (FALSE == StepperMotor_Start(&displacementMotor->stepperMotor, BACKWARD, displacementMotor->returnStepCount + MAXOFFLIMITSTEP, FALSE, DisplacementMotor_MoveToSensorOffLimitHandle))
            {
                TRACE_ERROR("\n Because no idle timer , the %s fail.", DisplacementMotor_GetName(number));
                DisplacementMotor_Stop(displacementMotor, RESULT_FAILED);
                return;
            }
        }
        if (StepperMotor_IDLE == StepperMotor_GetStatus(&displacementMotor->stepperMotor))
        {
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);

            //更新步数是为让复位过程被停止，步数还是相对正确的。
            Uint32 alreadyStep = StepperMotor_GetAlreadyStep(&displacementMotor->stepperMotor);
            displacementMotor->currentSteps -= alreadyStep;
            if (displacementMotor->currentSteps < 0)
            {
                displacementMotor->currentSteps = 0;
            }
            TRACE_INFO("\n %s backward Step : %d new currentSteps: %d", DisplacementMotor_GetName(number), alreadyStep, displacementMotor->currentSteps);

            if (TRUE == displacementMotor->isRequestStop)
            {
                DisplacementMotor_Stop(displacementMotor, RESULT_STOPPED);
            }
            else
            {
                if (FALSE == DisplacementMotor_IsSensorBlocked(number)) //未遮住传感器
                {
                    TRACE_ERROR("\n Because error no find sensor, the %s fail. Position sensor don't Blocked", DisplacementMotor_GetName(number));
                    if(X_DISPLACEMENTMOTOR == number)
                    {
                        if (TRUE == DisplacementMotor_CheckCollision(0))
                        {
                            DisplacementMotor_Stop(displacementMotor, RESULT_COLLISION_0); //X轴移向传感器碰撞
                        }
                        else if (TRUE == DisplacementMotor_CheckCollision(1))
                        {
                            DisplacementMotor_Stop(displacementMotor, RESULT_COLLISION_1); //X轴移向传感器碰撞
                        }
                        else
                        {
                            DisplacementMotor_Stop(displacementMotor, RESULT_MOVE_IN_SENSOR_FAIL_X); //X轴找不到传感器
                        }
                    }
                    else if(Z_DISPLACEMENTMOTOR == number)
                    {
                        DisplacementMotor_Stop(displacementMotor, RESULT_MOVE_IN_SENSOR_FAIL_Z);  //Z轴找不到传感器
                    }
                    else
                    {
                        DisplacementMotor_Stop(displacementMotor, RESULT_FAILED);
                    }
                }
                else
                {
                    displacementMotor->status = MOTOR_TO_ZERO;
                    displacementMotor->isStatusSwitchStart = TRUE;
                }
            }
        }
    }
    else if (MOTOR_TO_ZERO == displacementMotor->status)
    {
        if (TRUE == displacementMotor->isStatusSwitchStart)
        {
            displacementMotor->isStatusSwitchStart = FALSE;
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);
            TRACE_DEBUG("\n %s status: MOTOR_TO_ZERO", DisplacementMotor_GetName(number));
            StepperMotor_OtherMoveHandler limitHandle = NULL;
            if (number == Z_DISPLACEMENTMOTOR)
            {
                limitHandle = DisplacementMotor_ZMoveToZerosOffLimitHandle;
            }
            else
            {
                limitHandle = DisplacementMotor_XMoveToZerosOffLimitHandle;
            }

            if (FALSE == StepperMotor_Start(&displacementMotor->stepperMotor, FORWARD, displacementMotor->maxSteps + MAXOFFLIMITSTEP, FALSE, limitHandle))
            {
                TRACE_ERROR("\n Because no idle timer , the %s fail.", DisplacementMotor_GetName(number));
                DisplacementMotor_Stop(displacementMotor, RESULT_FAILED);
                return;
            }
        }
        if (StepperMotor_IDLE == StepperMotor_GetStatus(&displacementMotor->stepperMotor))
        {
            Uint8 number = StepperMotor_GetNumber(&displacementMotor->stepperMotor);

            Uint32 alreadyStep = StepperMotor_GetAlreadyStep(&displacementMotor->stepperMotor);
            displacementMotor->currentSteps += alreadyStep;

            MoveResult moveResult;
            if (TRUE == displacementMotor->isRequestStop)
            {
                moveResult = RESULT_STOPPED;
            }
            else
            {
                if (TRUE == DisplacementMotor_CheckCollision(0))
                {
                    TRACE_ERROR("\n Motor Move to Zero Fail. Collide check sensor 0 is Collision");
                    moveResult = RESULT_COLLISION_0;
                }
                else if (TRUE == DisplacementMotor_CheckCollision(1))
                {
                    TRACE_ERROR("\n  Motor Move to Zero Fail.  Collide check sensor 1 is Collision");
                    moveResult = RESULT_COLLISION_1;
                }
                else if (number == Z_DISPLACEMENTMOTOR && TRUE == DisplacementMotor_IsSensorBlocked(Z_DISPLACEMENTMOTOR))
                {
                    TRACE_ERROR("\n Because error no find sensor, the Z DisplacementMotor reset fail. Position sensor is Blocked");
                    moveResult = RESULT_MOVE_OUT_SENSOR_FAIL_Z;
                }
                else if (number == X_DISPLACEMENTMOTOR && TRUE == DisplacementMotor_IsSensorBlocked(X_DISPLACEMENTMOTOR))
                {
                    TRACE_ERROR("\n Because error no find sensor, the X DisplacementMotor reset fail. Position sensor is Blocked");
                    moveResult = RESULT_MOVE_OUT_SENSOR_FAIL_X;
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&displacementMotor->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    moveResult = RESULT_DRIVER_ERROR;
                }
                else
                {
                    TRACE_INFO("\n %s DisplacementMotor reset finish.", DisplacementMotor_GetName(number));
                    displacementMotor->currentSteps = 0;
                    moveResult = RESULT_FINISHED;
                }
            }
            TRACE_INFO("\n %s forward Step : %d new currentSteps: %d", DisplacementMotor_GetName(number), alreadyStep, displacementMotor->currentSteps);
            DisplacementMotor_Stop(displacementMotor, moveResult);
        }
    }
}

