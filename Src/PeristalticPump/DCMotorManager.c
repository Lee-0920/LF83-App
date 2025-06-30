/*
 * DCMotorManager.c
 *
 *  Created on: 2018年3月9日
 *      Author: LIANG
 */
#include "string.h"
#include "DCMotorManager.h"
#include "Driver/LiquidDriver/DCMotorMap.h"
#include "SystemConfig.h"
#include "DNCP/App/DscpSysDefine.h"
#include "DNCP/App/DscpSysDefine.h"
#include "PumpEventScheduler.h"
#include "DncpStack/DncpStack.h"

DCMotor g_DCMotor[DCMOTOR_TOTAL_PUMP];

static void DCMotorManager_TimerHandler(TimerHandle_t timer);
static void DCMotorManager_Stop(Uint8 index, MoveResult moveResult);

void DCMotorManager_Init()
{
    memset(g_DCMotor, 0, sizeof(g_DCMotor));
    DCMotorMap_Init(g_DCMotor);

    for (Uint8 i = 0; i < DCMOTOR_TOTAL_PUMP; i++)
    {
        g_DCMotor[i].timer = xTimerCreate("DCMotorManager",
                (uint32_t) (1 / portTICK_RATE_MS), pdTRUE, (void *) DCMOTOR_TIMER_PRIO,
                DCMotorManager_TimerHandler);
        g_DCMotor[i].isRunning = FALSE;
        g_DCMotor[i].isSendEvent = FALSE;
        g_DCMotor[i].startTime = 0;
        g_DCMotor[i].number = i;
    }
}
void DCMotorManager_Restore()
{
    for(Uint8 i = 0; i < DCMOTOR_TOTAL_PUMP; i++)
    {
        g_DCMotor[i].isSendEvent = FALSE;
        DCMotorManager_RequestStop(i);
    }
}

Uint16 DCMotorManager_Start(Uint8 index, float volume)
{
    if (index < DCMOTOR_TOTAL_PUMP && volume > 0)
    {
        if (FALSE == g_DCMotor[index].isRunning)
        {
            g_DCMotor[index].isRunning = TRUE;
            DCMotorDriver_Start(&g_DCMotor[index].driver);
            xTimerChangePeriod( g_DCMotor[index].timer, (uint32_t)((volume * 1000) / portTICK_RATE_MS), 0);//修改定时器周期
            xTimerStart(g_DCMotor[index].timer, 0);// 启动定时器， 0 表示不阻塞
            g_DCMotor[index].startTime = xTaskGetTickCount();
            TRACE_INFO("\n DCMotor %d start, volume: ", index);
            System_PrintfFloat(TRACE_LEVEL_INFO, volume, 4);
            TRACE_INFO(" ml");
            DncpStack_ClearBufferedEvent();
            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR("\n DCMotor is running, the DCMotor %d starts to fail.", index);
            return DSCP_BUSY;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n Parameter error, the DCMotor %d starts to fail. ", index);
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

Uint16 DCMotorManager_RequestStop(Uint8 index)
{
    if (index < DCMOTOR_TOTAL_PUMP)
    {
        if(StepperMotor_BUSY == DCMotorManager_IsRunning(index))
        {
            TRACE_INFO("\n DCMotor %d RequestStop", index);
            DCMotorManager_Stop(index, RESULT_STOPPED);
            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR("\n DCMotor %d not run, stop failure.", index);
            return DSCP_ERROR;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n Parameter error, the DCMotor %d stop to fail. ", index);
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

static void DCMotorManager_SendEvent(DCMotor *motor, MoveResult moveResult)
{
    if(TRUE == motor->isSendEvent)
    {
        enum PumpResult pumpResult = (enum PumpResult)moveResult;
        Uint8 data[2] = {0};
        data[0] = motor->number + DCMOTOR_OFFSET;
        data[1] = pumpResult;
        PumpEventScheduler_SendEvent(data[0], pumpResult, TRUE);
        DncpStack_BufferEvent(DSCP_EVENT_PPI_PUMP_RESULT, data , sizeof(data));
    }
    motor->isSendEvent = FALSE;
}

void DCMotorManager_Stop(Uint8 index, MoveResult moveResult)
{
    xTimerStop(g_DCMotor[index].timer, 0);
    DCMotorDriver_Stop(&g_DCMotor[index].driver);
    g_DCMotor[index].endTime = xTaskGetTickCount();
    TRACE_INFO("\n DCMotor %d stop, volume: ", index);
    System_PrintfFloat(TRACE_LEVEL_INFO, (float)((g_DCMotor[index].endTime - g_DCMotor[index].startTime) * 1.0 / 1000), 4);
    TRACE_INFO(" ml");
    DCMotorManager_SendEvent(&g_DCMotor[index], moveResult);
    g_DCMotor[index].isRunning = FALSE;
}

void DCMotorManager_TimerHandler(TimerHandle_t timer)
{
    Uint8 index;
    for( index = 0; index < DCMOTOR_TOTAL_PUMP; index++)
    {
        if (g_DCMotor[index].timer == timer)
        {
            break;
        }
    }
    if (index == DCMOTOR_TOTAL_PUMP)
    {
        TRACE_ERROR(
                "\n This timer does not exist , the DCMotor stop fail. ");
        return;
    }
    DCMotorManager_Stop(index, RESULT_FINISHED);
}


Uint16 DCMotorManager_GetTotalPumps()
{
    return DCMOTOR_TOTAL_PUMP;
}

float DCMotorManager_GetVolume(Uint8 index)
{
    if (index < DCMOTOR_TOTAL_PUMP)
    {
        if (FALSE == g_DCMotor[index].isRunning)
        {
            return (float)((g_DCMotor[index].endTime - g_DCMotor[index].startTime) * 1.0 / 1000);
        }
        else
        {
            return (float)((xTaskGetTickCount() - g_DCMotor[index].startTime) * 1.0 / 1000);
        }
    }
    else
    {
        TRACE_ERROR("\n No. %d DCMotor.", index);
        return 0;
    }
}

Bool DCMotorManager_SendEventOpen(Uint8 index)
{
    if (index < DCMOTOR_TOTAL_PUMP)
    {
        g_DCMotor[index].isSendEvent = TRUE;
        return TRUE;
    }
    else
    {
        TRACE_ERROR("\n No. %d DCMotor.", index);
        return FALSE;
    }
}

Bool DCMotorManager_IsRunning(Uint8 index)
{
    if (index < DCMOTOR_TOTAL_PUMP)
    {
        return g_DCMotor[index].isRunning;
    }
    else
    {
        TRACE_ERROR("\n No. %d DCMotor.", index);
        return FALSE;
    }
}
