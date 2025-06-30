/*
 * MotorControl.c
 *
 *  Created on: 2018年3月8日
 *      Author: LIANG
 */

#include "Tracer/Trace.h"
#include "Dncp/App/DscpSysDefine.h"
#include <string.h>
#include "PeristalticPump/DisplacementMotorManager.h"
#include "MotorControl.h"
#include "PeristalticPump/PeristalticPumpManager.h"
#include "PeristalticPump/SyringeManager.h"
#include "PeristalticPump/TMCConfig.h"

void MotorControl_GetTotalPumps(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 totalPumps = DISPLACEMENTMOTOR_TOTAL_PUMP;
    TRACE_INFO("\n MotorControl totalPumps: %d", totalPumps);
    DscpDevice_SendResp(dscp, &totalPumps, sizeof(Uint16));
}

void MotorControl_GetMotionParam(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    StepperMotorParam param ;
    memcpy(&index, data, sizeof(Uint8));
    param = DisplacementMotor_GetDefaultMotionParam(index);

    TRACE_INFO("\n %s acc:", DisplacementMotor_GetName(index));
    System_PrintfFloat(TRACE_LEVEL_INFO, param.acceleration, 4);
    TRACE_INFO(" step/(s^2), maxSpeed:");
    System_PrintfFloat(TRACE_LEVEL_INFO, param.maxSpeed, 4);
    TRACE_INFO(" step/s");

    DscpDevice_SendResp(dscp, &param, sizeof(StepperMotorParam));
}

void MotorControl_SetMotionParam(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    StepperMotorParam param;
    Uint16 size = 0;
    Uint8 index;

    //设置数据正确性判断
    size = sizeof(StepperMotorParam) + sizeof(Uint8);
    if ((len > size))
    {
        ret = DSCP_ERROR;
        TRACE_ERROR("Parame Len Error\n");
        TRACE_ERROR("%d \n", size);
    }
    else
    {
        memcpy(&index, data, sizeof(Uint8));
        memcpy(&param, data + sizeof(Uint8), sizeof(StepperMotorParam));
        ret = DisplacementMotor_SetDefaultMotionParam(index, param);
    }
    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_GetStatus(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    Uint16 ret = DSCP_IDLE;
    memcpy(&index, data, sizeof(Uint8));

    if (MOTOR_IDLE != DisplacementMotor_GetStatus(index))
    {
        ret = DSCP_BUSY;
    }

    if (ret == DSCP_IDLE)
    {
        TRACE_INFO("\n %s idle", DisplacementMotor_GetName(index));
    }
    else
    {
        TRACE_INFO("\n %s busy", DisplacementMotor_GetName(index));
    }
    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_GetMaxSteps(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    Uint16 step ;
    memcpy(&index, data, sizeof(Uint8));
    step = DisplacementMotor_GetMaxSteps(index);

    TRACE_INFO("\n %s step: %d", DisplacementMotor_GetName(index), step);

    DscpDevice_SendResp(dscp, &step, sizeof(step));
}

void MotorControl_GetInitSteps(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    Uint16 step ;
    memcpy(&index, data, sizeof(Uint8));
    step = 0;

    TRACE_INFO("\n %s step: %d", DisplacementMotor_GetName(index), step);

    DscpDevice_SendResp(dscp, &step, sizeof(step));
}

void MotorControl_GetCurrentSteps(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    Uint16 step ;
    memcpy(&index, data, sizeof(Uint8));
    step = DisplacementMotor_GetCurrentSteps(index);

    TRACE_INFO("\n %s step: %d", DisplacementMotor_GetName(index), step);

    DscpDevice_SendResp(dscp, &step, sizeof(step));
}

void MotorControl_Start(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    Uint16 size = 0;
    Uint8 index;
    Int16 steps;
    DisplacementMotorMode mode;

    //设置数据正确性判断
    size = sizeof(Uint8) + sizeof(Int16) + sizeof(Uint8);
    if ((len > size))
    {
        ret = DSCP_ERROR;
        TRACE_ERROR("Parame Len Error\n");
        TRACE_ERROR("%d \n", size);
    }
    else
    {
        memcpy(&index, data, sizeof(Uint8));
        size = sizeof(Uint8);
        memcpy(&steps, data + size, sizeof(Int16));
        size += sizeof(Int16);
        memcpy(&mode, data + size, sizeof(DisplacementMotorMode));

        DisplacementMotor_SendEventOpen(index);
        ret = DisplacementMotor_Start(index, steps, mode, TRUE);
    }
    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_Stop(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    Uint8 index;

    memcpy(&index, data, sizeof(Uint8));

    DisplacementMotor_SendEventOpen(index);
    ret = DisplacementMotor_RequestStop(index);

    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_Reset(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    Uint8 index;

    memcpy(&index, data, sizeof(Uint8));

    DisplacementMotor_SendEventOpen(index);
    ret = DisplacementMotor_Reset(index);

    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_GetSensorStatus(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_IDLE;
    Uint8 index;
    memcpy(&index, data, sizeof(Uint8));
    switch(index)
    {
    case 0:
        if (TRUE == SyringeManager_IsSensorBlocked())
        {
            TRACE_INFO("\n Syringe Sensor Blocked");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n Syringe Sensor don't Blocked");
        }
        break;
    case 1:
        if (TRUE == DisplacementMotor_IsSensorBlocked(X_DISPLACEMENTMOTOR))
        {
            TRACE_INFO("\n XDisplacementMotor Sensor Blocked");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n XDisplacementMotor Sensor don't Blocked");
        }
        break;
    case 2:
        if (TRUE == DisplacementMotor_IsSensorBlocked(Z_DISPLACEMENTMOTOR))
        {
            TRACE_INFO("\n ZDisplacementMotor Sensor Blocked");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n ZDisplacementMotor Sensor don't Blocked");
        }
        break;
    case 3:
        if (TRUE == DisplacementMotor_CheckCollision(0))
        {
            TRACE_INFO("\n 0 Collision Sensor don't Blocked! Collide!");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n 0 Collision Sensor Blocked.");
        }
        break;
    case 4:
        if (TRUE == DisplacementMotor_CheckCollision(1))
        {
            TRACE_INFO("\n 1 Collision Sensor don't Blocked! Collide!");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n 1 Collision Sensor Blocked.");
        }
        break;
    case 5:
        if (TRUE == SyringeManager_IsWaterCheckSensorBlocked(0))
        {
            TRACE_INFO("\n Water Check Sensor 0 Blocked");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n Water Check Sensor 0 don't Blocked");
        }
        break;
    case 6:
        if (TRUE == SyringeManager_IsWaterCheckSensorBlocked(1))
        {
            TRACE_INFO("\n Water Check Sensor 1 Blocked");
            ret = DSCP_BUSY;
        }
        else
        {
            TRACE_INFO("\n Water Check Sensor 1 don't Blocked");
        }
        break;
    }

    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_DriverReinit(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;

    TMCConfig_Reinit();

    DscpDevice_SendStatus(dscp, ret);
}

void MotorControl_DriverCheck(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 ret = -1;
    Uint8 index;
    memcpy(&index, data, sizeof(Uint8));
    switch(index)
    {
        case 0:
            ret = TMCConfig_DriverCheck(DisplacementMotor_GetStepperMotorX());
            break;
        case 1:
            ret = TMCConfig_DriverCheck(DisplacementMotor_GetStepperMotorZ());
            break;
        case 2:
            ret = TMCConfig_DriverCheck(SyringeManager_GetStepperMotor());
            break;
        case 3:
            ret = TMCConfig_DriverCheck(PeristalticPumpManager_GetStepperMotor(0));
            break;
        default:
            break;
    }

    DscpDevice_SendResp(dscp, &ret, sizeof(Uint8));
}
