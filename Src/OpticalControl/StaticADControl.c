/*
 * StaticADControl.c
 *
 *  Created on: 2020年1月6日
 *      Author: Administrator
 */

#include "FreeRTOS.h"
#include "task.h"
#include "SystemConfig.h"
#include "Driver/McuFlash.h"
#include "DncpStack/DncpStack.h"
#include "Tracer/Trace.h"
#include "StaticADControl.h"
#include "PMTControl.h"
#include <math.h>
#include "LuipApi/OpticalAcquireInterface.h"
#include <OpticalDriver/AD5175Driver.h>
#include <OpticalDriver/PMTDriver.h>

typedef enum
{
    ADCTL_IDLE,
    ADCTL_BUSY
} ADCtrlStatus;

typedef enum
{
    POSITIVE_CORRELATION,    //正相关-电阻越大 AD越大
    NEGATIVE_CORRELATION    //负相关-电阻越大 AD越小
}ADCtrlCorrelation;

typedef struct
{
    AD5175Driver* ad5175;
    Uint16  defaultValue;
    Uint8   addr;
}ADController;

static StaticADControlResult s_adCtrlResult;
static Bool s_adCtrlSendEvent;
static ADCtrlStatus s_adCtrlStatus;
static Uint8 s_currentIndex;
static Uint32 s_targetAD;
static Uint32 s_currentAD;
static Uint32 s_lastAD;
static Uint16 s_currentValue;
static Uint16 s_lastValue;
static Uint16 s_minValue;
static Uint16 s_maxValue;
static Bool s_adCtrlOver;
static AD5175Driver ad5175_iic1;
static AD5175Driver ad5175_iic2;
static ADController g_adController[AD_CONTROLLER_NUM];
static Bool s_isValid = FALSE;
static ADCtrlCorrelation s_correlation = NEGATIVE_CORRELATION;

static void StaticADControl_ADHandleTask(void *argument);
static Uint32 StaticADControl_GetCurrentAD(Uint8 index);

static xTaskHandle s_staticADControlHandle;

void StaticADControl_Init(void)
{
    StaticADControl_InitDriver();
    StaticADControl_InitParam();
    StaticADControl_InitSetting();

    xTaskCreate(StaticADControl_ADHandleTask, "StaticADControlHandle",
            STATIC_AD_CONTROL_STK_SIZE, NULL,
            STATIC_AD_CONTROL_TASK_PRIO, &s_staticADControlHandle);
}

void StaticADControl_InitDriver(void)
{
    ad5175_iic1.pinSCL = GPIO_Pin_12;
    ad5175_iic1.portSCL = GPIOE;
    ad5175_iic1.rccSCL  = RCC_AHB1Periph_GPIOE;
    ad5175_iic1.pinSDA = GPIO_Pin_13;
    ad5175_iic1.portSDA = GPIOE;
    ad5175_iic1.rccSDA  = RCC_AHB1Periph_GPIOE;
    AD5175_Init(&ad5175_iic1);

    g_adController[PMT_MEA].ad5175 = &ad5175_iic1;
    g_adController[PMT_MEA].addr = AD5175_ADDR_1;

    ad5175_iic2.pinSCL = GPIO_Pin_14;
    ad5175_iic2.portSCL = GPIOE;
    ad5175_iic2.rccSCL  = RCC_AHB1Periph_GPIOE;
    ad5175_iic2.pinSDA = GPIO_Pin_15;
    ad5175_iic2.portSDA = GPIOE;
    ad5175_iic2.rccSDA  = RCC_AHB1Periph_GPIOE;
    AD5175_Init(&ad5175_iic2);

    g_adController[PMT_REF].ad5175 = &ad5175_iic2;
    g_adController[PMT_REF].addr = AD5175_ADDR_1;
}

void StaticADControl_InitParam(void)
{
    Uint8 buffer[AD5175_CONTROL_SIGN_FLASH_LEN] = { 0 };
    Uint32 flashFactorySign = 0;
    Uint8 param[AD_CONTROLLER_NUM*2] = {0};
    Uint8 i;

    McuFlash_Read(AD5175_CONTROL_SIGN_FLASH_BASE_ADDR, AD5175_CONTROL_SIGN_FLASH_LEN, buffer);                //读取出厂标志位
    memcpy(&flashFactorySign, buffer, AD5175_CONTROL_SIGN_FLASH_LEN);

    if (FLASH_FACTORY_SIGN == flashFactorySign)       //表示已经过出厂设置
    {
        for(i = 0; i < AD_CONTROLLER_NUM; i++)
        {
            g_adController[i].defaultValue = StaticADControl_GetDefaultValue(i);
        }
    }
    else
    {
        //未出厂,使用设备默认值
        for(i = 0; i < AD_CONTROLLER_NUM; i++)
        {
            g_adController[i].defaultValue = AD5175_ReadRDAC(g_adController[i].ad5175, g_adController[i].addr);
            memcpy(&param[i*2], &g_adController[i].defaultValue, sizeof(Uint16));
        }
        //保存设备默认值至Flash
        McuFlash_Write(AD5175_CONTROL_PARAM_FLASH_BASE_ADDR, AD5175_CONTROL_PARAM_FLASH_LEN, param);

        //写入出厂标志
        flashFactorySign = FLASH_FACTORY_SIGN;
        memcpy(buffer, &flashFactorySign, AD5175_CONTROL_SIGN_FLASH_LEN);

        McuFlash_Write(AD5175_CONTROL_SIGN_FLASH_BASE_ADDR, AD5175_CONTROL_SIGN_FLASH_LEN, buffer);
    }
}

void StaticADControl_InitSetting(void)
{
    ///PMTControl_PowerOn();
    PMTDriver_PowerOn();
    System_NonOSDelay(5);

    s_isValid = FALSE;
    for(Uint8 i = 0; i < AD_CONTROLLER_NUM; i++)
    {
        if(TRUE == AD5175_WriteRDAC(g_adController[i].ad5175, g_adController[i].addr, g_adController[i].defaultValue))     //使用默认值设置AD5175
        {
            s_isValid = TRUE;
            TRACE_INFO("\n static ad control is valid.");
        }
    }

    PMTControl_PowerOff();
}

Bool StaticADControl_LoadSetting(void)
{
    Bool ret = TRUE;
    for(Uint8 i = 0; i < AD_CONTROLLER_NUM; i++)
    {
        if(FALSE == AD5175_WriteRDAC(g_adController[i].ad5175, g_adController[i].addr, g_adController[i].defaultValue))   //使用默认值设置AD5175
        {
            ret = FALSE;
        }
    }

    return ret;
}

Bool StaticADControl_Start(Uint8 index, Uint32 targetAD)
{
    int timeout = 1000;
    PmtResult result;

    if(index < AD_CONTROLLER_NUM)
    {
        if (ADCTL_IDLE == s_adCtrlStatus)
        {
            if(PMT_IDLE == PMTControl_GetStatus())
            {
                s_correlation = NEGATIVE_CORRELATION;
                s_currentIndex = index;
                s_targetAD = targetAD;
                s_currentAD = 0xFFFFFFFF;
                s_lastAD = s_currentAD;
                s_currentValue =  g_adController[s_currentIndex].defaultValue;
                s_lastValue = 0xFFFF;
                s_maxValue = AD5175_MAX_VALUE;
                s_minValue = AD5175_MIN_VALUE;
                s_adCtrlOver = FALSE;
                s_adCtrlResult = STATIC_AD_CONTROL_RESULT_UNFINISHED;
                s_adCtrlStatus = ADCTL_BUSY;

                PMTControl_PowerOn();  //开启PMT板电源

                s_currentValue =  g_adController[s_currentIndex].defaultValue;
                AD5175_WriteRDAC(g_adController[s_currentIndex].ad5175, g_adController[s_currentIndex].addr, s_currentValue);

                PMTControl_StartAcquire(200, FALSE);
                while(FALSE == PMTControl_GetNewResult(&result))
                {
                    System_Delay(10);
                    timeout -= 10;
                    if(timeout < 0)
                    {
                        TRACE_ERROR("\nPMT Read timeout !");
                        return FALSE;
                    }
                }
                if(s_currentIndex == PMT_REF)
                {
                    s_currentAD = 5 * result.refValue;
                }
                else
                {
                    s_currentAD = 5 * result.meaValue;
                }
                s_lastAD = s_currentAD;

                TRACE_INFO("\n static AD control start. index = %d, targetAD = %d, startAD = %d, startValue = %d", index, targetAD, s_currentAD, s_currentValue);
                vTaskResume(s_staticADControlHandle);
                return TRUE;
            }
            else
            {
                TRACE_ERROR("\n PMT control handle is busy.");
                return FALSE;
            }
        }
        else
        {
            TRACE_ERROR("\n static AD control failed to start because it is running.");
            return FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n static AD control failed to start because index must between 0 - %d.", AD_CONTROLLER_NUM);
        return FALSE;
    }
}

void StaticADControl_Stop(void)
{
    if (ADCTL_BUSY == s_adCtrlStatus)
    {
        if(s_adCtrlOver == TRUE)
        {
            s_adCtrlResult =  STATIC_AD_CONTROL_RESULT_FINISHED;
        }
        else
        {
            s_adCtrlResult =  STATIC_AD_CONTROL_RESULT_UNFINISHED;
        }

        PMTControl_StopAcquire(FALSE);

        if(s_adCtrlSendEvent == TRUE)
        {
            // 发送结果事件
            DncpStack_SendEvent(DSCP_EVENT_OAI_STATIC_AD_CONTROL_RESULT, &s_adCtrlResult, sizeof(StaticADControlResult));
            DncpStack_BufferEvent(DSCP_EVENT_OAI_STATIC_AD_CONTROL_RESULT, &s_adCtrlResult, sizeof(StaticADControlResult));
            StaticADControl_SendEventClose();  //关闭事件发送
            TRACE_INFO("\n static ad control send result = %d addr = %0x", (Uint8)s_adCtrlResult, &s_adCtrlResult);
            System_Delay(50);
        }

        if(s_adCtrlOver == TRUE)
        {
            if(s_currentValue != g_adController[s_currentIndex].defaultValue)
            {
                StaticADControl_SetDefaultValue(s_currentIndex, s_currentValue);
            }
            else
            {
                TRACE_INFO("\n control result value is equal to default value");
            }
        }

        PMTControl_PowerOff();  //关闭PMT板电源

        s_adCtrlOver = FALSE;
        s_adCtrlStatus = ADCTL_IDLE;
        s_correlation = NEGATIVE_CORRELATION;
        TRACE_INFO("\n static ad control stop.");
    }
    else
    {
        TRACE_INFO("\n static ad controller is not running.");
    }
}

Uint16 StaticADControl_GetDefaultValue(Uint8 index)
{
    Uint8 buffer[2] = {0};
    Uint16 offset = 0;
    Uint16 value = 0xFFFF;

    if(index < AD_CONTROLLER_NUM)
    {
        offset = index*(sizeof(Uint16));

        McuFlash_Read(AD5175_CONTROL_PARAM_FLASH_BASE_ADDR + offset, sizeof(Uint16), buffer);

        memcpy(&value, buffer, sizeof(Uint16));

        TRACE_DEBUG("\n static ad ctrl get  index %d default value %d.", index, value);
    }
    else
    {
        TRACE_ERROR("\n invalid index %d.", index);
    }

    return value;
}

void StaticADControl_SetDefaultValue(Uint8 index, Uint16 value)
{
    Uint8 buffer[2] = {0};
    Uint16 offset = 0;
    Uint16 useValue = value&AD5175_MAX_VALUE;

    if(index < AD_CONTROLLER_NUM)
    {
        offset = index*(sizeof(Uint16));

        memcpy(buffer, &useValue, sizeof(Uint16));

        McuFlash_Write(AD5175_CONTROL_PARAM_FLASH_BASE_ADDR + offset, sizeof(Uint16), buffer);

        g_adController[index].defaultValue = useValue;

        TRACE_INFO("\n static ad ctrl set  index %d default value %d.", index, value);
    }
    else
    {
        TRACE_ERROR("\n invalid index %d.", index);
    }
}

Uint16 StaticADControl_GetRealValue(Uint8 index)
{
    Uint16 value = 0xFFFF;

    if(index < AD_CONTROLLER_NUM)
    {
        value = AD5175_ReadRDAC(g_adController[index].ad5175, g_adController[index].addr);
        TRACE_INFO("\n static ad ctrl get  index %d current value %d.", index, value);
    }
    else
    {
        TRACE_ERROR("\n invalid index %d  return %d.", index, value);
    }

    return value;
}

Bool StaticADControl_SetRealValue(Uint8 index, Uint16 value)
{
    if(AD5175_WriteRDAC(g_adController[index].ad5175, g_adController[index].addr, value))
    {
        TRACE_INFO("\n static ad ctrl set  index %d real value %d.", index, value);
        return TRUE;
    }
    else
    {
        TRACE_ERROR("\n static ad ctrl set  real value fail.");
        return FALSE;
    }
}

static Uint32 StaticADControl_GetCurrentAD(Uint8 index)
{
    Uint8 filterNum = 5;
    Uint32 ad = 0;
    PmtResult result;
    int timeout = 3000;

    switch(index)
    {
        case PMT_REF:
            for(int i = 0; i < filterNum; i++)
            {
                timeout = 3000;
                PMTControl_StartAcquire(1000, FALSE);
                while((FALSE == PMTControl_GetNewResult(&result)) && (timeout > 0))
                {
                    System_Delay(10);
                    timeout -= 10;
                }
                if(timeout <= 0)
                {
                    TRACE_ERROR("\nGet pmt result timeout");
                }

                ad = ad + result.refValue;
                TRACE_CODE("\n current PMT_REF ad[%d] = %d", i, result.refValue);
                System_Delay(10);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current PMT_REF ad average  = %d", ad);
            break;

        case PMT_MEA:
            for(int i = 0; i < filterNum; i++)
            {
                timeout = 3000;
                PMTControl_StartAcquire(1000, FALSE);
                while((FALSE == PMTControl_GetNewResult(&result)) && (timeout > 0))
                {
                    System_Delay(10);
                    timeout -= 10;
                }
                if(timeout <= 0)
                {
                    TRACE_ERROR("\nGet pmt result timeout");
                }

                ad = ad + result.meaValue;
                TRACE_CODE("\n current PMT_MEA ad[%d] = %d", i, result.meaValue);
                System_Delay(10);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current PMT_MEA ad average  = %d", ad);
            break;

        default:
            break;
    }

    return ad;
}

void StaticADControl_ADHandleTask(void *argument)
{
    vTaskSuspend(NULL);
    while (1)
    {
        switch (s_adCtrlStatus)
        {
            case ADCTL_IDLE:
                vTaskSuspend(NULL);
                break;

            case ADCTL_BUSY:
                s_currentAD = StaticADControl_GetCurrentAD(s_currentIndex);
                TRACE_DEBUG("\n static AD control current value = %d, ad = %d", s_currentValue, s_currentAD);
                if(fabs((double)s_lastValue - (double)s_currentValue)  < 2 || fabs((double)s_currentAD - (double)s_targetAD) <= 0.01*(double)s_targetAD)
                {
                    if(fabs((double)s_lastAD - (double)s_targetAD) <= fabs((double)s_currentAD - (double)s_targetAD))
                    {
                        s_currentValue = s_lastValue;
                    }
                    s_adCtrlOver = TRUE;
                }
                else
                {
                    if(POSITIVE_CORRELATION == s_correlation)  //正相关
                    {
                        if(s_currentAD <= s_targetAD)    //电阻越大 AD越大
                        {
                            s_minValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_maxValue)/2;
                        }
                        else
                        {
                            s_maxValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_minValue)/2;
                        }
                    }
                    else if(NEGATIVE_CORRELATION == s_correlation)  //负相关
                    {
                        if(s_currentAD <= s_targetAD)    //电阻越小 AD越大
                        {
                            s_maxValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_minValue)/2;
                        }
                        else
                        {
                            s_minValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_maxValue)/2;
                        }
                    }

                    s_adCtrlOver = FALSE;
                }
                TRACE_DEBUG("\n calculate new value = %d", s_currentValue);
                if(FALSE == AD5175_WriteRDAC(g_adController[s_currentIndex].ad5175, g_adController[s_currentIndex].addr, s_currentValue)) //写失败后停止
                {
                    StaticADControl_Stop();
                }

                if(s_adCtrlOver == TRUE)
                {
                    TRACE_INFO("\n static AD control over. value = %d", s_currentValue);

                    StaticADControl_Stop();
                }
                System_Delay(1000);
                break;
        }
    }
}

void StaticADControl_SendEventOpen(void)
{
    s_adCtrlSendEvent = TRUE;
}

void StaticADControl_SendEventClose(void)
{
    s_adCtrlSendEvent = FALSE;
}

Bool StaticADControl_IsValid(void)
{
    return s_isValid;
}


