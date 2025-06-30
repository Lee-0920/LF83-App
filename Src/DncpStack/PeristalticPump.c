/*
 * PeristalticPump.c
 *
 *  Created on: 2016年6月4日
 *      Author: Administrator
 */
#include "Tracer/Trace.h"
#include "Dncp/App/DscpSysDefine.h"
#include <string.h>
#include "PeristalticPump/PeristalticPumpManager.h"
#include "PeristalticPump/SyringeManager.h"
#include "System.h"
#include "PeristalticPump.h"
#include "SystemConfig.h"
#include <stdio.h>
#include "PeristalticPump/DCMotorManager.h"

typedef enum
{
    SYRINGE,
    PERISTALTICPUMP,
    DCMOTOR,
    MAX_PERISTALTICPUMPTYPE,
}PeristalticPumpType;

static PeristalticPumpType PeristalticPump_GetType(Uint8 index)
{
    PeristalticPumpType peristalticPumpType;
    if (index < PERISTALTICPUMP_OFFSET)//注射器
    {
        peristalticPumpType = SYRINGE;
    }
    else if (index < DCMOTOR_OFFSET)//蠕动泵
    {
        peristalticPumpType = PERISTALTICPUMP;
    }
    else if (index < DNCP_PUMP_TOTAL)//直流电机
    {
        peristalticPumpType = DCMOTOR;
    }
    else//不存在
    {
        peristalticPumpType = MAX_PERISTALTICPUMPTYPE;
    }
    return peristalticPumpType;
}

static char* PeristalticPump_GetInfo(Uint8 index)
{
    static char name[25] = "";
    memset(name, 0, sizeof(name));

    switch(PeristalticPump_GetType(index))
    {
    case SYRINGE:
        strcpy(name, "Syringe");
        break;
    case PERISTALTICPUMP:
        {
            Uint8 number = index - PERISTALTICPUMP_OFFSET;
            sprintf(name, "PeristalticPump %d", number);
        }
        break;
    case DCMOTOR:
        {
            Uint8 number = index - DCMOTOR_OFFSET;
            sprintf(name, "DCMotor %d", number);
        }
        break;
    default:
        strcpy(name, "NULL");
        break;
    }
    return name;
}

/**
 * @brief 查询系统支持的总泵数目。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_GetTotalPumps(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 totalPumps = DNCP_PUMP_TOTAL;
    TRACE_INFO("\n totalPumps: %d", totalPumps);
    DscpDevice_SendResp(dscp, &totalPumps, sizeof(Uint16));
}

/**
 * @brief 查询指定泵的运动参数。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_GetMotionParam(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    StepperMotorParam param;
    memcpy(&index, data, sizeof(Uint8));

    switch(PeristalticPump_GetType(index))
    {
    case SYRINGE:
        param = SyringeManager_GetDefaultMotionParam();
        break;
    case PERISTALTICPUMP:
        param = PeristalticPumpManager_GetDefaultMotionParam(index - PERISTALTICPUMP_OFFSET);
        break;
    default:
        break;
    }
    TRACE_INFO("\n index:%d, %s acc:", index, PeristalticPump_GetInfo(index));
    System_PrintfFloat(TRACE_LEVEL_INFO, param.acceleration, 4);
    TRACE_INFO(" ml/(s^2), maxSpeed:");
    System_PrintfFloat(TRACE_LEVEL_INFO, param.maxSpeed, 4);
    TRACE_INFO(" ml/s");

    DscpDevice_SendResp(dscp, &param, sizeof(StepperMotorParam));
}

/**
 * @brief 设置指定泵的运动参数。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_SetMotionParam(DscpDevice* dscp, Byte* data, Uint16 len)
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
        switch(PeristalticPump_GetType(index))
        {
        case SYRINGE:
            ret = SyringeManager_SetDefaultMotionParam(param);
            break;
        case PERISTALTICPUMP:
            ret = PeristalticPumpManager_SetDefaultMotionParam(index - PERISTALTICPUMP_OFFSET, param);
            break;
        case MAX_PERISTALTICPUMPTYPE:
            ret = (Uint16)DSCP_ERROR_PARAM;
            break;
        default:
            break;
        }
    }
    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 查询指定泵的校准系数。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_GetFactor(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    float factor = 0;
    memcpy(&index, data, sizeof(Uint8));

    switch(PeristalticPump_GetType(index))
    {
    case SYRINGE:
        factor = SyringeManager_GetFactor();
        break;
    case PERISTALTICPUMP:
        factor = PeristalticPumpManager_GetFactor(index - PERISTALTICPUMP_OFFSET);
        break;
    default:
        break;
    }
    TRACE_INFO("\n index:%d, %s get factor:", index, PeristalticPump_GetInfo(index));
    System_PrintfFloat(TRACE_LEVEL_INFO, factor, 8);
    TRACE_INFO(" ml/step");

    DscpDevice_SendResp(dscp, &factor, sizeof(float));
}

/**
 * @brief 设置指定泵的校准系数。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_SetFactor(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    float factor;
    Uint16 size = 0;
    Uint8 index;

    //设置数据正确性判断
    size = sizeof(float) + sizeof(Uint8);
    if ((len > size))
    {
        ret = DSCP_ERROR;
        TRACE_ERROR("Parame Len Error\n");
        TRACE_ERROR("%d \n", size);
    }
    else
    {
        memcpy(&index, data, sizeof(Uint8));
        memcpy(&factor, data + sizeof(Uint8), sizeof(float));
        switch(PeristalticPump_GetType(index))
        {
        case SYRINGE:
            ret = SyringeManager_SetFactor(factor);
            break;
        case PERISTALTICPUMP:
            ret = PeristalticPumpManager_SetFactor(index - PERISTALTICPUMP_OFFSET, factor);
            break;
        case MAX_PERISTALTICPUMPTYPE:
            ret = (Uint16)DSCP_ERROR_PARAM;
            break;
        default:
            break;
        }
    }
    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 查询指定泵的工作状态。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_GetStatus(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    Uint16 ret = DSCP_IDLE;
    memcpy(&index, data, sizeof(Uint8));

    switch(PeristalticPump_GetType(index))
    {
    case SYRINGE:
        if (SYRINGE_IDLE != SyringeManager_GetStatus())
        {
            ret = DSCP_BUSY;
        }
        break;
    case PERISTALTICPUMP:
        if (StepperMotor_IDLE != PeristalticPumpManager_GetStatus(index - PERISTALTICPUMP_OFFSET))
        {
            ret = DSCP_BUSY;
        }
        break;
    case DCMOTOR:
        if (TRUE == DCMotorManager_IsRunning(index - DCMOTOR_OFFSET))
        {
            ret = DSCP_BUSY;
        }
        break;
    default:
        break;
    }

    if (ret == DSCP_IDLE)
    {
        TRACE_INFO("\n index:%d, %s idle", index, PeristalticPump_GetInfo(index));
    }
    else
    {
        TRACE_INFO("\n index:%d, %s busy", index, PeristalticPump_GetInfo(index));
    }
    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 查询泵出的体积。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_GetVolume(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    float volume = 0;
    memcpy(&index, data, sizeof(Uint8));

    switch(PeristalticPump_GetType(index))
    {
    case PERISTALTICPUMP:
        volume = PeristalticPumpManager_GetVolume(index - PERISTALTICPUMP_OFFSET);
        break;
    case DCMOTOR:
        volume = DCMotorManager_GetVolume(index - DCMOTOR_OFFSET);
        break;
    default:
        break;
    }
    TRACE_INFO("\n index:%d, %s Volume", index, PeristalticPump_GetInfo(index));
    System_PrintfFloat(TRACE_LEVEL_INFO, volume, 4);
    TRACE_INFO(" ml");
    DscpDevice_SendResp(dscp, &volume, sizeof(float));
}

/**
 * @brief 启动泵。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_Start(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    Direction dir;
    float volume;
    float speed;
    Uint16 size = 0;
    Uint8 index;

    //设置数据正确性判断
    size = sizeof(Uint8) + sizeof(Direction) + sizeof(float) + sizeof(float);
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
        memcpy(&dir, data + size, sizeof(Direction));
        size += sizeof(Direction);
        memcpy(&volume, data + size, sizeof(float));
        size += sizeof(float);
        memcpy(&speed, data + size, sizeof(float));

        switch(PeristalticPump_GetType(index))
        {
        case SYRINGE:
            SyringeManager_SendEventOpen();
            if (speed > 0)
            {
                StepperMotorParam param;
                param = SyringeManager_GetDefaultMotionParam();
                param.maxSpeed = speed;
                ret = SyringeManager_SetCurrentMotionParam(param);
                if (DSCP_OK == ret)
                {
                    ret = SyringeManager_Start(dir, volume, FALSE);
                }
            }
            else
            {
                ret = SyringeManager_Start(dir, volume, TRUE);
            }
            break;
        case PERISTALTICPUMP:
            {
                Uint8 number = index - PERISTALTICPUMP_OFFSET;
                PeristalticPumpManager_SendEventOpen(number);
                if (speed > 0)
                {
                    StepperMotorParam param;
                    param = PeristalticPumpManager_GetDefaultMotionParam(number);
                    param.maxSpeed = speed;
                    ret = PeristalticPumpManager_SetCurrentMotionParam(number, param);
                    if (DSCP_OK == ret)
                    {
                        ret = PeristalticPumpManager_Start(number, dir, volume, FALSE);
                    }
                }
                else
                {
                    ret = PeristalticPumpManager_Start(number, dir, volume, TRUE);
                }
            }
            break;
        case DCMOTOR:
            DCMotorManager_SendEventOpen(index - DCMOTOR_OFFSET);
            ret = DCMotorManager_Start(index - DCMOTOR_OFFSET, volume);
            break;
        case MAX_PERISTALTICPUMPTYPE:
            ret = (Uint16)DSCP_ERROR_PARAM;
            break;
        }
    }
    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 停止泵。
 * @param dscp
 * @param data
 * @param len
 */
void PeristalticPump_Stop(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    Uint8 index;

    memcpy(&index, data, sizeof(Uint8));

    switch(PeristalticPump_GetType(index))
    {
    case SYRINGE:
        SyringeManager_SendEventOpen();
        ret = SyringeManager_RequestStop();
        break;
    case PERISTALTICPUMP:
        PeristalticPumpManager_SendEventOpen(index - PERISTALTICPUMP_OFFSET);
        ret = PeristalticPumpManager_Stop(index - PERISTALTICPUMP_OFFSET);
        break;
    case DCMOTOR:
        DCMotorManager_SendEventOpen(index - DCMOTOR_OFFSET);
        ret = DCMotorManager_RequestStop(index - DCMOTOR_OFFSET);
        break;
    case MAX_PERISTALTICPUMPTYPE:
        ret = (Uint16)DSCP_ERROR_PARAM;
        break;
    }

    DscpDevice_SendStatus(dscp, ret);
}

