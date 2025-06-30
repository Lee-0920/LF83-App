/*
 * OpticalAcquire.c
 *
 *  Created on: 2017年11月22日
 *      Author: LIANG
 */

#include "DncpStack/OpticalAcquire.h"
#include <string.h>
#include "Common/Utils.h"
#include "DNCP/App/DscpSysDefine.h"
#include "Tracer/Trace.h"
#include "Driver/System.h"
#include "DncpStack/DncpStack.h"
#include "OpticalControl/PMTControl.h"
#include "OpticalControl/StaticADControl.h"

void OpticalAcquire_SetSignalADNotifyPeriod(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_NOT_SUPPORTED;

    DscpDevice_SendStatus(dscp, ret);
}

void OpticalAcquire_StartAcquirer(DscpDevice* dscp, Byte* data, Uint16 len)
{
    unsigned short ret  = DSCP_OK;
    float samplePeriod = 0;
    Uint32 timeout = 1000;

    // 采样时间
    memcpy(&samplePeriod, data, sizeof(float));
    timeout = (Uint32)(samplePeriod * 1000);
    TRACE_INFO("\n Time %d ms ", timeout);
    if(FALSE == PMTControl_StartAcquire(timeout, TRUE))
    {
        ret = DSCP_ERROR;
    }

    DscpDevice_SendStatus(dscp, ret);
}

void OpticalAcquire_StopAcquirer(DscpDevice* dscp, Byte* data, Uint16 len)
{
    unsigned short ret = DSCP_OK;

    PMTControl_StopAcquire(TRUE);

    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 设置运放增益
 * @param index Uint8，通道索引, 参考0,测量1
 * @param gainCoefficient Uint8，运放增益,0-7
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquire_SetGain(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_NOT_SUPPORTED;

    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 获取运放增益。
 * @param index Uint8，通道索引, 参考0,测量1
 * @return gain，Uint8，0-7
 */
void OpticalAcquire_GetGain(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 gain = 0;

    DscpDevice_SendResp(dscp, &gain, sizeof(Uint8));
}

/**
 * @brief 开启PMT板电源
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquire_PowerOn(DscpDevice* dscp, Byte* data, Uint16 len)
{
    unsigned short ret = DSCP_OK;

    PMTControl_PowerOn();

    if(FALSE == StaticADControl_LoadSetting())  //设置出错时再设一次
    {
        TRACE_WARN("\nStatic AD Try Reset!");
        System_Delay(5);
        if(FALSE == StaticADControl_LoadSetting())
        {
            TRACE_WARN("\nStatic AD Load Fail!");
            ret = DSCP_ERROR;
        }
    }

    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 关闭PMT板电源
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquire_PowerOff(DscpDevice* dscp, Byte* data, Uint16 len)
{
    unsigned short ret = DSCP_OK;

    PMTControl_PowerOff();

    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 启动PMT增益调节
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquirer_StartStaticADControl(DscpDevice* dscp, Byte* data, Uint16 len)
{
    unsigned short ret = DSCP_OK;
    Uint8 index;
    Uint32 targetAD;

    int size = 0;
    //设置数据正确性判断
    size =  sizeof(Uint8)  + sizeof(Uint32);
    if ((len > size))
    {
        ret = DSCP_ERROR;
        TRACE_ERROR("Parame Len Error\n");
        TRACE_ERROR("%d \n", size);
    }
    else
    {
        memcpy(&index, data, sizeof(Uint8));
        memcpy(&targetAD, data+sizeof(Uint8), sizeof(Uint32));
        //TRACE_DEBUG("StaticAD Control  index = %d, target = %d", index, targetAD);
        if (TRUE == StaticADControl_Start(index, targetAD))
        {
            StaticADControl_SendEventOpen();
        }
        else
        {
            ret = DSCP_ERROR;
        }
    }

    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 停止PMT增益调节
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquirer_StopStaticADControl(DscpDevice* dscp, Byte* data, Uint16 len)
{
    unsigned short ret = DSCP_OK;
    StaticADControl_SendEventOpen();
    StaticADControl_Stop();
    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 获取PMT增益参数
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquirer_GetStaticADControlParam(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint8 index;
    Uint16 value = DSCP_ERROR;

    int size = 0;
    //设置数据正确性判断
    size =  sizeof(Uint8);
    if ((len > size))
    {
        TRACE_ERROR("param error len : %d > %d\n", len, size);
    }
    else
    {
        memcpy(&index, data, sizeof(Uint8));

        if(index < AD_CONTROLLER_NUM)
        {
            value = StaticADControl_GetDefaultValue(index);
        }
        else
        {
            TRACE_ERROR("param index error \n");
        }
    }

    DscpDevice_SendResp(dscp, &value, sizeof(Uint16));
}

/**
 * @brief 设置PMT增益参数
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquirer_SetStaticADControlParam(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_OK;
    Uint16 size = 0;
    Uint8 index;
    Uint16 value;

    size = sizeof(Uint8) + sizeof(Uint16);
    if ((len > size))
    {
        ret = DSCP_ERROR;
        TRACE_ERROR("param error len : %d > %d\n", len, size);
    }
    else
    {
        memcpy(&index, data, sizeof(Uint8));
        memcpy(&value, data+sizeof(Uint8), sizeof(Uint16));

        if(index < AD_CONTROLLER_NUM)
        {
            PMTControl_PowerOn();  //开启PMT板电源

            StaticADControl_SetDefaultValue(index, value);
            if(StaticADControl_SetRealValue(index, value) == FALSE)
            {
                ret = DSCP_ERROR;
            }

            PMTControl_PowerOff();  //关闭PMT板电源
        }
        else
        {
            TRACE_ERROR("param index error \n");
            ret = DSCP_ERROR;
        }
    }
    DscpDevice_SendStatus(dscp, ret);
}

/**
 * @brief 查询PMT增益调节是否有效
 * @return 状态回应，Uint16，支持的状态有：
 *  - @ref DSCP_OK  操作成功；
 *  - @ref DSCP_ERROR 操作失败；
 */
void OpticalAcquire_IsADControlValid(DscpDevice* dscp, Byte* data, Uint16 len)
{
    Uint16 ret = DSCP_ERROR;
    if(TRUE == StaticADControl_IsValid())
    {
        ret = DSCP_OK;
    }
    DscpDevice_SendStatus(dscp, ret);
}

