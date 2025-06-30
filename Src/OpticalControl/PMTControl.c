/*
 * PMTControl.c
 *
 *  Created on: 2019年11月4日
 *      Author: Administrator
 */

#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "queue.h"
#include "PMTControl.h"
#include "LuipApi/OpticalAcquireInterface.h"
#include "Driver/OpticalDriver/PMTDriver.h"
#include "DncpStack/DncpStack.h"
#include "Common/Types.h"
#include "SystemConfig.h"
#include "Tracer/Trace.h"
#include "semphr.h"
#include "System.h"

typedef enum
{
    AcquireFailed = 0,
    AcquireSucceed = 1,
    AcquireStopped = 2,
}PmtEvent;

// 定时器
#define PMT_CONTROL_TIMER_RCC                     RCC_APB1Periph_TIM12
#define PMT_CONTROL_TIMER_IRQn                    TIM8_BRK_TIM12_IRQn
#define PMT_CONTROL_IRQHANDLER                  TIM8_BRK_TIM12_IRQHandler
#define PMT_CONTROL_TIMER                               TIM12

//最大定时时间(ms)
#define PMT_CONTROL_MAX_TIME  12000

static Uint32 s_acquireTime = 0;   //ms
static Bool s_isSendEvent = FALSE;
static Bool s_isRequestStop = FALSE;

static PmtResult s_pmtResult = {0,0};
static Bool s_newResult = FALSE;
static AcquiredResult s_resultCode = ACQUIRED_RESULT_FAILED;

static PMT_STATUS s_pmtStatus = PMT_IDLE;
static Bool s_pmtPower = FALSE;

static TickType_t s_startTime;
static TickType_t s_realTime;

static Uint32 s_powerTimes = 0;

static Bool s_isAcquireFinished = FALSE;



/* ----------------------- Task ----------------------------------------*/
xTaskHandle s_pmtAcquireTask;
static void PMTAcquire_Handle(TaskHandle_t argument);
static void PMTControl_TimerInit(void);

/**
 * @brief PMT控制器初始化
 */
void PMTControl_Init(void)
{
    PMTDriver_Init();

    PMTControl_TimerInit();

    s_isAcquireFinished = FALSE;

    xTaskCreate(PMTAcquire_Handle, "PMTAcquire", PMT_ACQUIRE_STK_SIZE, NULL, PMT_ACQUIRE_TASK_PRIO, &s_pmtAcquireTask);

    s_pmtStatus = PMT_IDLE;
}

/**
 * @brief PMT控制定时器初始化
 */
static void PMTControl_TimerInit(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStrecture;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(PMT_CONTROL_TIMER_RCC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = PMT_CONTROL_TIMER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PMT_CONTROL_TIMER_IQR_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_TimeBaseInitStrecture.TIM_Period = 4999;        //1s
    TIM_TimeBaseInitStrecture.TIM_Prescaler = 4499;  //17999;            //0.2ms;
    TIM_TimeBaseInitStrecture.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStrecture.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStrecture.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(PMT_CONTROL_TIMER, &TIM_TimeBaseInitStrecture);

    TIM_ARRPreloadConfig(PMT_CONTROL_TIMER, DISABLE);
    TIM_ClearFlag(PMT_CONTROL_TIMER, TIM_FLAG_Update);
    TIM_ITConfig(PMT_CONTROL_TIMER, TIM_IT_Update, DISABLE);
    TIM_Cmd(PMT_CONTROL_TIMER, DISABLE);
}

/**
 * @brief PMT定时器启动
 * @param uint16_t time, 单位ms
 */
static void PMTControl_TimerStart(Uint16 time)
{
    Uint16 value = time*5 - 1;

    TIM_SetAutoreload(PMT_CONTROL_TIMER, value);

    TIM_ClearFlag(PMT_CONTROL_TIMER, TIM_FLAG_Update);
    TIM_ITConfig(PMT_CONTROL_TIMER, TIM_IT_Update, ENABLE);
    TIM_Cmd(PMT_CONTROL_TIMER, ENABLE);
}

/**
 * @brief PMT定时器停止
 */
static void PMTControl_TimerStop(void)
{
    TIM_ITConfig(PMT_CONTROL_TIMER, TIM_IT_Update, DISABLE);
    TIM_Cmd(PMT_CONTROL_TIMER, DISABLE);
}

/**
 * @brief PMT控制定时器中断服务函数
 */
void PMT_CONTROL_IRQHANDLER(void)
{
     if(TIM_GetITStatus(PMT_CONTROL_TIMER, TIM_IT_Update) == SET)
     {
         TIM_ClearITPendingBit(PMT_CONTROL_TIMER, TIM_IT_Update);

         PMTDriver_DisableRef();
         PMTDriver_DisableMea();

         s_pmtResult.refValue = PMTDriver_ReadCounterRef();
         s_pmtResult.meaValue = PMTDriver_ReadCounterMea();

         PMTControl_TimerStop();

         TRACE_INFO("\nPMT Driver Result: Ref = %d, Mea = %d", s_pmtResult.refValue, s_pmtResult.meaValue);

         s_resultCode = ACQUIRED_RESULT_FINISHED;

         s_isAcquireFinished = TRUE;
     }
}

/**
 * @brief PMT信号采集
 * @param[in] timeout 超时时间, 单位 ms
 */
Bool PMTControl_StartAcquire(Uint32 timeout, Bool isSendEvent)
{
    if(s_pmtStatus == PMT_IDLE)
    {
        if(timeout > PMT_CONTROL_MAX_TIME)
        {
            TRACE_ERROR("\nPMT Once Max Acquire Time is %d ms", PMT_CONTROL_MAX_TIME);
            return FALSE;
        }
        else if(timeout <= 0)
        {
            TRACE_ERROR("\nAcquire Time %d ms is invalid", timeout);
            return FALSE;
        }

        s_acquireTime = timeout;
        s_isSendEvent = isSendEvent;
        s_isRequestStop = FALSE;
        s_resultCode = ACQUIRED_RESULT_FAILED;
        s_pmtResult.refValue = 0;
        s_pmtResult.meaValue = 0;
        s_newResult = FALSE;

        s_isAcquireFinished = FALSE;

        TRACE_ERROR("\nPMT Acquire Start total time = %d ms", timeout);

        s_pmtStatus = PMT_BUSY;
        s_startTime = xTaskGetTickCount();
        vTaskResume(s_pmtAcquireTask);

        taskENTER_CRITICAL();

        PMTDriver_EnableRef();
        PMTDriver_EnableMea();

        PMTControl_TimerStart(s_acquireTime);

        taskEXIT_CRITICAL();

        return TRUE;
    }
    else
    {
        TRACE_ERROR("\nPMT Control is acquiring");
        return FALSE;
    }
}

/**
 * @brief 停止信号采集
 */
void PMTControl_StopAcquire(Bool isSendEvent)
{
    PMTControl_TimerStop();
    PMTDriver_DisableRef();
    PMTDriver_DisableMea();

    s_isSendEvent = isSendEvent;
    s_isRequestStop = TRUE;
    s_pmtResult.refValue = 0;
    s_pmtResult.meaValue = 0;

    s_acquireTime = 0;
    s_resultCode = ACQUIRED_RESULT_STOPPED;

    s_isAcquireFinished = TRUE;

    TRACE_INFO("\nPMT Acquire Stop");
}

/**
 * @brief 获取最新PMT采集数据
 */
Bool PMTControl_GetNewResult(PmtResult* result)
{
    if(s_newResult == TRUE)
    {
        result->refValue = s_pmtResult.refValue;
        result->meaValue = s_pmtResult.meaValue;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief PMT是否空闲
 */
Uint8 PMTControl_GetStatus(void)
{
    return (Uint8)s_pmtStatus;
}

/**
 * @brief PMT电源开启
 */
void PMTControl_PowerOn()
{
    PMTDriver_PowerOn();
    s_pmtPower = TRUE;

    ///System_NonOSDelay(5);
    System_Delay(5);

    s_powerTimes++;
    TRACE_INFO("\nOpen PMT  power times %d", s_powerTimes);
}

/**
 * @brief PMT电源关闭
 */
void PMTControl_PowerOff()
{
    PMTDriver_PowerOff();
    s_pmtPower = FALSE;

    TRACE_INFO("\nClose PMT  power");
}

/**
 * @brief PMT电源状态
 */
Bool PMTControl_PowerStatus()
{
    return s_pmtPower;
}

/**
  * @brief PMT测试
  */
void PMTAcquire_Handle(TaskHandle_t argument)
{
    vTaskSuspend(NULL);
    while(1)
    {
        switch(s_pmtStatus)
        {
            case PMT_IDLE:
                s_startTime = xTaskGetTickCount();
                break;

            case PMT_BUSY:
                s_realTime = xTaskGetTickCount() - s_startTime;

                if(s_realTime < s_acquireTime*2*portTICK_RATE_MS)  //成功或者被停止
                {
                    if ( TRUE == s_isAcquireFinished)
                    {
                        TRACE_INFO("\nGet PMT RESULT CODE = %d", s_resultCode);

                        s_isAcquireFinished = FALSE;

                        TRACE_INFO("\nPMT Acuire Result: Ref = %d, Mea = %d, Acquire Time = %d ms, Real Time = %d ms", s_pmtResult.refValue, s_pmtResult.meaValue, s_acquireTime, s_realTime);

                        s_newResult = TRUE;

                        if (TRUE == s_isSendEvent)
                        {
                            Uint8 data[9] = {0};
                            memcpy(data, &s_pmtResult, sizeof(s_pmtResult));
                            data[8] = s_resultCode;
                            DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                            DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                        }

                        s_isRequestStop = FALSE;
                        s_isSendEvent = FALSE;
                        s_pmtStatus = PMT_IDLE;
                    }
                }
                else  //超时
                {
                    TRACE_INFO("\nPMT Acuire Timeout");
                    s_resultCode = ACQUIRED_RESULT_FAILED;

                    TRACE_INFO("\nPMT Acuire Result: Ref = %d, Mea = %d, Acquire Time = %d ms, Real Time = %d ms", s_pmtResult.refValue, s_pmtResult.meaValue, s_acquireTime, s_realTime);

                    s_newResult = TRUE;

                    if (TRUE == s_isSendEvent)
                    {
                        Uint8 data[9] = {0};
                        memcpy(data, &s_pmtResult, sizeof(s_pmtResult));
                        data[8] = s_resultCode;
                        DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                        DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                    }

                    s_isRequestStop = FALSE;
                    s_isSendEvent = FALSE;
                    s_pmtStatus = PMT_IDLE;
                }
                break;

            default:
                break;
        }


/*
        switch(s_pmtStatus)
        {
            case PMT_IDLE:
                vTaskSuspend(s_pmtAcquireTask);
                break;

            case PMT_BUSY:
                s_startTime = xTaskGetTickCount();
                if(PMTControl_ResultGet(s_acquireTime*2) == TRUE)  //成功或者被停止
                {
                    TRACE_INFO("\nGet PMT RESULT CODE = %d", s_resultCode);
                }
                else  //超时
                {
                    TRACE_INFO("\nPMT Acuire Timeout");
                    s_resultCode = ACQUIRED_RESULT_FAILED;
                }
                s_realTime = xTaskGetTickCount() - s_startTime;
                TRACE_INFO("\nPMT Acuire Result: Ref = %d, Mea = %d, Acquire Time = %d ms, Real Time = %d ms", s_pmtResult.refValue, s_pmtResult.meaValue, s_acquireTime, s_realTime);

                s_newResult = TRUE;

                if (TRUE == s_isSendEvent)
                {
                    Uint8 data[9] = {0};
                    memcpy(data, &s_pmtResult, sizeof(s_pmtResult));
                    data[8] = s_resultCode;
                    DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                    DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                }

                s_isRequestStop = FALSE;
                s_isSendEvent = FALSE;
                s_pmtStatus = PMT_IDLE;
                break;

            default:
                break;
        }
        */
        vTaskDelay(10);
    }
}
