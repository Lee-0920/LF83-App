
#include "DNCP/App/DscpSysDefine.h"
#include "PumpEventScheduler.h"
#include "LiquidDriver/StepperMotorMap.h"
#include "SystemConfig.h"
#include "McuFlash.h"
#include "DncpStack/DncpStack.h"
#include "Driver/LiquidDriver/PositionSensorMap.h"
#include "SyringeManager.h"
#include "StepperMotor.h"

#define LIMITSTEPMAX 4
#define WATERSENSORNUM 2

Syringe g_syringe;
PositionSensor g_waterCheckSensor[WATERSENSORNUM];

/*********Task*********/
static void SyringeManager_TaskHandle(void *pvParameters);
/*********Function*********/
static void SyringeManager_MoveToSensorOffLimitHandle(void *obj) ;
static void SyringeManager_MoveToZerosOffLimitHandle(void *obj);
static void SyringeManager_SendEvent(Syringe *syringe, MoveResult moveResult);
static void SyringeManager_Stop(Syringe *syringe, MoveResult moveResult);

void SyringeManager_Init()
{
    memset(&g_syringe, 0, sizeof(Syringe));

    StepperMotorMap_SyringeInit(&g_syringe);
    PositionSensorMap_SyringeInit(&g_syringe);
    PositionSensorMap_WaterCheckSensorInit(g_waterCheckSensor);

    StepperMotor_SetSubdivision(&g_syringe.stepperMotor, 16);

    Uint8 buffer[SYRINGE_SIGN_FLASH_LEN] = { 0 };
    Uint32 flashFactorySign = 0;
    StepperMotorParam param;

    McuFlash_Read(SYRINGE_SIGN_FLASH_BASE_ADDR,
            SYRINGE_SIGN_FLASH_LEN, buffer); //读取出厂标志位
    memcpy(&flashFactorySign, buffer, SYRINGE_SIGN_FLASH_LEN);
    if (FLASH_FACTORY_SIGN == flashFactorySign) //表示已经过出厂设置
    {
        g_syringe.factor = SyringeManager_GetFactor();
        param = SyringeManager_GetDefaultMotionParam();
    }
    else
    {
        float factor;
        factor = 0.0005;//0.5ul/step
        SyringeManager_SetFactor(factor);

        param.acceleration = 400 * factor;
        param.maxSpeed = 200 * factor;
        SyringeManager_SetDefaultMotionParam(param);

        flashFactorySign = FLASH_FACTORY_SIGN;
        memcpy(buffer, &flashFactorySign, SYRINGE_SIGN_FLASH_LEN);
        McuFlash_Write(
                SYRINGE_SIGN_FLASH_BASE_ADDR,
                SYRINGE_SIGN_FLASH_LEN, buffer);
    }

    param.acceleration = param.acceleration / g_syringe.factor;
    param.maxSpeed = param.maxSpeed / g_syringe.factor;
    StepperMotor_SetNumber(&g_syringe.stepperMotor, 2);
    StepperMotor_Init(&g_syringe.stepperMotor, param);

    g_syringe.isSendEvent = FALSE;
    g_syringe.isStatusSwitchStart = FALSE;
    g_syringe.isRequestStop = FALSE;
    g_syringe.limitStepCount = 0;
    g_syringe.status = SYRINGE_IDLE;

    xTaskCreate(SyringeManager_TaskHandle, "SyringeHandle", SYRINGE_STK_SIZE, (void *)&g_syringe,
            SYRINGE_TASK_PRIO, &g_syringe.xHandle);
}

void SyringeManager_Restore(void)
{
    g_syringe.isSendEvent = FALSE;
    SyringeManager_RequestStop();
//    while(SYRINGE_IDLE != SyringeManager_GetStatus())
//    {
//        vTaskDelay(5 / portTICK_RATE_MS);
//    }
//    SyringeManager_Start(BACKWARD, 0.5, TRUE);
}

StepperMotor* SyringeManager_GetStepperMotor(void)
{
    return &g_syringe.stepperMotor;
}

StepperMotorParam SyringeManager_GetDefaultMotionParam()
{
    StepperMotorParam param;
    memset(&param, 0, sizeof(StepperMotorParam));

    Uint8 readData[SYRINGE_MOTIONPARAM_FLASH_LEN] ={ 0 };

    McuFlash_Read(SYRINGE_MOTIONPARAM_FLASH_BASE_ADDR,
            SYRINGE_MOTIONPARAM_FLASH_LEN, readData);

    memcpy(&param, readData, sizeof(StepperMotorParam));
    return param;
}

StepperMotorParam SyringeManager_GetCurrentMotionParam()
{
    StepperMotorParam param;
    memset(&param, 0, sizeof(StepperMotorParam));

    param = StepperMotor_GetCurrentMotionParam(&g_syringe.stepperMotor);
    param.acceleration = param.acceleration * g_syringe.factor;
    param.maxSpeed = param.maxSpeed * g_syringe.factor;
    return param;
}

Uint16 SyringeManager_SetDefaultMotionParam(StepperMotorParam param)
{
    float lowSpeed = STEPPERMOTOR_MIN_SUBDIVISION_SPEED  / StepperMotor_GetSubdivision(&g_syringe.stepperMotor) * g_syringe.factor;
    float highSpeed = STEPPERMOTOR_MAX_SUBDIVISION_SPEED / StepperMotor_GetSubdivision(&g_syringe.stepperMotor) * g_syringe.factor;

    if (0 != param.acceleration
            && (param.maxSpeed >= lowSpeed)
            && (param.maxSpeed <= highSpeed)
            && 0 != g_syringe.factor)
    {
        TRACE_INFO("\n Syringe set default param acc :");
        System_PrintfFloat(TRACE_LEVEL_INFO, param.acceleration, 4);
        TRACE_INFO(" ml/(s^2),maxSpeed:");
        System_PrintfFloat(TRACE_LEVEL_INFO, param.maxSpeed, 4);
        TRACE_INFO(" ml/s,factor");
        System_PrintfFloat(TRACE_LEVEL_INFO, g_syringe.factor, 4);
        TRACE_INFO(" ml/step\n");

        StepperMotorParam setparam;
        setparam.acceleration = param.acceleration / g_syringe.factor;
        setparam.maxSpeed = param.maxSpeed / g_syringe.factor;

        if (TRUE == StepperMotor_SetDefaultMotionParam(&g_syringe.stepperMotor, setparam))
        {
            Uint8 writeData[SYRINGE_MOTIONPARAM_FLASH_LEN] ={ 0 };
            memcpy(writeData, &param, sizeof(StepperMotorParam));
            McuFlash_Write(
                    SYRINGE_MOTIONPARAM_FLASH_BASE_ADDR,
                    SYRINGE_MOTIONPARAM_FLASH_LEN, writeData);
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
                "\n The Syringe motion default parameter setting failed because of the parameter error.");

        TRACE_ERROR("\n Syringe set default param maxSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, param.maxSpeed, 4);
        TRACE_ERROR(" ml/s,acc:");
        System_PrintfFloat(TRACE_LEVEL_ERROR, param.acceleration, 4);
        TRACE_ERROR(" ml/(s^2)\n factor:");
        System_PrintfFloat(TRACE_LEVEL_ERROR, g_syringe.factor, 4);
        TRACE_ERROR(" ml/step\n lowSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, lowSpeed, 4);
        TRACE_ERROR(" ml/s\n highSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, highSpeed, 4);
        TRACE_ERROR(" ml/s\n");

        return ((Uint16) DSCP_ERROR_PARAM);
    }

}

Uint16 SyringeManager_SetCurrentMotionParam(StepperMotorParam param)
{

    float lowSpeed = STEPPERMOTOR_MIN_SUBDIVISION_SPEED  / StepperMotor_GetSubdivision(&g_syringe.stepperMotor) * g_syringe.factor;
    float highSpeed = STEPPERMOTOR_MAX_SUBDIVISION_SPEED / StepperMotor_GetSubdivision(&g_syringe.stepperMotor) * g_syringe.factor;

    if (0 != param.acceleration
     && (param.maxSpeed >= lowSpeed)
     && (param.maxSpeed <= highSpeed)
     && 0 != g_syringe.factor)
    {
        TRACE_INFO("\n Syringe set current param maxSpeed:");
        System_PrintfFloat(TRACE_LEVEL_INFO, param.maxSpeed, 4);
        TRACE_INFO(" ml/s,acc:");
        System_PrintfFloat(TRACE_LEVEL_INFO, param.acceleration, 4);
        TRACE_INFO(" ml/(s^2) ");
        System_PrintfFloat(TRACE_LEVEL_INFO, g_syringe.factor, 4);
        TRACE_INFO(" ml/step\n");

        StepperMotorParam setparam;
        setparam.acceleration = param.acceleration / g_syringe.factor;
        setparam.maxSpeed = param.maxSpeed / g_syringe.factor;

        StepperMotor_SetCurrentMotionParam(&g_syringe.stepperMotor, setparam);
        return DSCP_OK;
    }
    else
    {
        TRACE_ERROR(
             "\n The Syringe motion current parameter setting failed because of the parameter error.");

        TRACE_ERROR("\n Syringe set current param maxSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, param.maxSpeed, 4);
        TRACE_ERROR(" ml/s,acc:");
        System_PrintfFloat(TRACE_LEVEL_ERROR, param.acceleration, 4);
        TRACE_ERROR(" ml/(s^2)\n factor:");
        System_PrintfFloat(TRACE_LEVEL_ERROR, g_syringe.factor, 4);
        TRACE_ERROR(" ml/step\n lowSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, lowSpeed, 4);
        TRACE_ERROR(" ml/s\n highSpeed :");
        System_PrintfFloat(TRACE_LEVEL_ERROR, highSpeed, 4);
        TRACE_ERROR(" ml/s\n");

        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

float SyringeManager_GetFactor()
{
    Uint8 readData[SYRINGE_FACTOR_FLASH_LEN] ={ 0 };
    float factor;

    //获取FLASH的系数
    McuFlash_Read(SYRINGE_FACTOR_FLASH_BASE_ADDR,
            SYRINGE_FACTOR_FLASH_LEN, readData);
    memcpy(&factor, readData, sizeof(float));
    g_syringe.factor = factor;
    return factor;
}

Uint16 SyringeManager_SetFactor(float factor)
{
    if (0 != factor)
    {
        if (SYRINGE_IDLE == g_syringe.status)
        {
            TRACE_INFO("\n Syringe set factor:");
            System_PrintfFloat(TRACE_LEVEL_INFO, factor, 8);
            TRACE_INFO(" ml/step");

            Uint8 writeData[SYRINGE_FACTOR_FLASH_LEN] ={ 0 };

            g_syringe.factor = factor;
            //系数写入FLASH
            memcpy(writeData, &factor, sizeof(float));
            McuFlash_Write(
            SYRINGE_FACTOR_FLASH_BASE_ADDR,
            SYRINGE_FACTOR_FLASH_LEN, writeData);

            StepperMotorParam setparam = SyringeManager_GetDefaultMotionParam();
            setparam.acceleration = setparam.acceleration / g_syringe.factor;
            setparam.maxSpeed = setparam.maxSpeed / g_syringe.factor;

            StepperMotor_SetDefaultMotionParam(&g_syringe.stepperMotor, setparam);

            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR(
            "\n The Syringe is running and can not change the calibration factor.");
            return DSCP_ERROR;
        }
    }
    else
    {
        TRACE_ERROR(
        "\n The Syringe calibration factor setting failed because of the parameter error.");
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

Bool SyringeManager_SendEventOpen()
{
    g_syringe.isSendEvent = TRUE;
    return TRUE;
}

SyringeStatus SyringeManager_GetStatus()
{
    return g_syringe.status;
}

Bool SyringeManager_IsSensorBlocked()
{
    if (SENSOR_HIGH_LEVEL == PositionSensor_ReadInputStatus(&g_syringe.positionSensor))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

Bool SyringeManager_IsWaterCheckSensorBlocked(Uint8 index)
{
    Bool ret;
    if (index < DISPLACEMENTMOTOR_TOTAL_PUMP)
    {
        if (SENSOR_HIGH_LEVEL == PositionSensor_ReadInputStatus(&g_waterCheckSensor[index]))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n No. %d Water Check Sensor.", index);
        ret = FALSE;
    }
    return ret;
}

/**
 * Description    : 移动到传感器的限制处理
 */
void SyringeManager_MoveToSensorOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    // 如果遮住传感器
    if (TRUE == SyringeManager_IsSensorBlocked())
    {
        g_syringe.limitStepCount++;
        if (g_syringe.limitStepCount >= LIMITSTEPMAX)
        {
            if(g_syringe.status == SYRINGE_DRAIN) //状态换成移动到传感器
            {
                g_syringe.status = SYRINGE_DRAIN_TO_SENSOR;
            }
            //停止运行
            StepperMotor_ImmediatelyStop(stepperMotor);
            TRACE_INFO("\n Syringe move to sensor succeed.PositionSensor status %d", PositionSensor_ReadInputStatus(&g_syringe.positionSensor));
        }
    }
    else
    {
        g_syringe.limitStepCount = 0;
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

/**
 * Description    : 移动到零点的限制处理
 */
void SyringeManager_MoveToZerosOffLimitHandle(void *obj)
{
    StepperMotor *stepperMotor = (StepperMotor *)obj;
    // 如果没遮住传感器
    if (FALSE == SyringeManager_IsSensorBlocked())
    {
        g_syringe.limitStepCount++;
        if (g_syringe.limitStepCount >= LIMITSTEPMAX)
        {
            //停止运行
            StepperMotor_ImmediatelyStop(stepperMotor);
            TRACE_INFO("\n Syringe move to zeros.PositionSensor status %d", PositionSensor_ReadInputStatus(&g_syringe.positionSensor));
        }
    }
    else
    {
        g_syringe.limitStepCount = 0;
    }

    //驱动报警诊断
    StepperMotor_DiagnosticCheck(stepperMotor);
}

/**
 * @brief 启动注射器。
 * @detail 注射器排空为原点位置，正常运动下传感器应该为未遮挡状态，排空到极限位置时为遮挡状态。
 */
Uint16 SyringeManager_Start(Direction dir, float volume, Bool isUseDefaultParam)
{
    if (volume > 0 && volume <= 1 && dir < MAX_DIRECTION)//单个注射器最大排量为1ml
    {
        if (SYRINGE_IDLE == g_syringe.status)
        {
            g_syringe.limitStepCount = 0;
            Uint32 step = (Uint32) (volume / g_syringe.factor + 0.5);
            if (FORWARD == dir)//吸入
            {
                TRACE_INFO("\n Syringe start extract volume ");
                System_PrintfFloat(TRACE_LEVEL_INFO, volume, 3);
                TRACE_INFO(" ml");
                if  (TRUE == StepperMotor_Start(&g_syringe.stepperMotor, FORWARD, step, isUseDefaultParam, StepperMotor_DiagnosticCheck))  ///<驱动诊断
                {
                    g_syringe.status = SYRINGE_EXTRACT;
                }
                else
                {
                    TRACE_ERROR("\n Because no idle timer , the Syringe starts to fail.");
                    return DSCP_ERROR;
                }
            }
            else//排出
            {
                TRACE_INFO("\n Syringe start drain or reset");
                if  (TRUE == StepperMotor_Start(&g_syringe.stepperMotor, BACKWARD, step, isUseDefaultParam, SyringeManager_MoveToSensorOffLimitHandle))
                {
                    if(step >= 2000)
                    {
                        StepperMotor_ChangeStep(&g_syringe.stepperMotor, 2100);
                        g_syringe.status = SYRINGE_DRAIN_TO_SENSOR;
                        TRACE_INFO("\n Syringe status：SYRINGE_DRAIN_TO_SENSOR");
                    }
                    else
                    {
                        g_syringe.status = SYRINGE_DRAIN;
                        TRACE_INFO("\n Syringe status：SYRINGE_DRAIN");
                    }
                }
                else
                {
                    TRACE_ERROR("\n Because no idle timer , the Syringe starts to fail.");
                    return DSCP_ERROR;
                }

            }
            g_syringe.isStatusSwitchStart = TRUE;
            g_syringe.isRequestStop = FALSE;
            DncpStack_ClearBufferedEvent();
            vTaskResume(g_syringe.xHandle);
            return DSCP_OK;
        }
        else
        {
            TRACE_ERROR("\n Because the pump is running, the Syringe starts to fail.");
            return DSCP_BUSY;
        }
    }
    else
    {
        TRACE_ERROR(
                "\n Parameter error, the Syringe starts fail. ");
        return ((Uint16) DSCP_ERROR_PARAM);
    }
}

Uint16 SyringeManager_RequestStop()
{
    if(SYRINGE_IDLE != g_syringe.status)
    {
        g_syringe.isRequestStop = TRUE;
        StepperMotor_RequestStop(&g_syringe.stepperMotor);
        return DSCP_OK;
    }
    else
    {
        TRACE_ERROR("\n Syringe not run, stop failure.");
        return DSCP_ERROR;
    }
}

void SyringeManager_SendEvent(Syringe *syringe, MoveResult moveResult)
{
    if(TRUE == syringe->isSendEvent)
    {
        enum PumpResult pumpResult = (enum PumpResult)moveResult;
        Uint8 data[2] = {0};
        data[0] = 0 + SYRINGE_OFFSET;//需要加上偏移量
        data[1] = pumpResult;
        PumpEventScheduler_SendEvent(data[0], pumpResult, TRUE);//需要加上偏移量
        DncpStack_BufferEvent(DSCP_EVENT_PPI_PUMP_RESULT, data , sizeof(data));
    }
    syringe->isSendEvent = FALSE;
}

void SyringeManager_Stop(Syringe *syringe, MoveResult moveResult)
{
    if((RESULT_FINISHED != moveResult) && (RESULT_STOPPED != moveResult))
    {
        StepperMotor_DriverCheck(&syringe->stepperMotor);
    }
    SyringeManager_SendEvent(syringe, moveResult);
    syringe->status = SYRINGE_IDLE;
    syringe->isStatusSwitchStart = TRUE;
    TRACE_INFO("\n Syringe Stop. result = %d", (Uint8)moveResult);
}

void SyringeManager_TaskHandle(void *pvParameters)
{
    Syringe *syringe = (Syringe *)pvParameters;
    vTaskSuspend(NULL);
    while (1)
    {
        vTaskDelay(5 / portTICK_RATE_MS);
        switch (syringe->status)
        {
        case SYRINGE_IDLE:
            vTaskSuspend(NULL);
            break;
        case SYRINGE_EXTRACT:
            if (StepperMotor_IDLE == StepperMotor_GetStatus(&syringe->stepperMotor))
            {
                MoveResult moveResult;
                if (TRUE == syringe->isRequestStop)
                {
                    moveResult = RESULT_STOPPED;
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&syringe->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    moveResult = RESULT_DRIVER_ERROR;
                }
                else
                {
                    moveResult = RESULT_FINISHED;
                }
                float alreadyVolume = (float) (StepperMotor_GetAlreadyStep(&syringe->stepperMotor) * syringe->factor);
                TRACE_INFO("\n Syringe extract volume:");
                System_PrintfFloat(TRACE_LEVEL_INFO, alreadyVolume, 4);
                TRACE_INFO(" ml");
                SyringeManager_Stop(syringe, moveResult);
            }
            break;
        case SYRINGE_DRAIN:
            if (StepperMotor_IDLE == StepperMotor_GetStatus(&syringe->stepperMotor))
            {
                MoveResult moveResult;
                if (TRUE == syringe->isRequestStop)
                {
                    moveResult = RESULT_STOPPED;
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&syringe->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    moveResult = RESULT_DRIVER_ERROR;
                }
                else
                {
                    moveResult = RESULT_FINISHED;
                }
                float alreadyVolume = (float) (StepperMotor_GetAlreadyStep(&syringe->stepperMotor) * syringe->factor);
                TRACE_INFO("\n Syringe drain volume:");
                System_PrintfFloat(TRACE_LEVEL_INFO, alreadyVolume, 4);
                TRACE_INFO(" ml");
                SyringeManager_Stop(syringe, moveResult);
            }
            break;
        case SYRINGE_DRAIN_TO_SENSOR:
            if (StepperMotor_IDLE == StepperMotor_GetStatus(&syringe->stepperMotor))
            {
                if (TRUE == syringe->isRequestStop)
                {
                    SyringeManager_Stop(syringe, RESULT_STOPPED);
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&syringe->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    SyringeManager_Stop(syringe, RESULT_DRIVER_ERROR);
                }
                else
                {
                    if (FALSE == SyringeManager_IsSensorBlocked())
                    {
                        TRACE_ERROR("\n Because error no find sensor, the Syringe fail. Photosensor don't Blocked");
                        SyringeManager_Stop(syringe, RESULT_MOVE_IN_SENSOR_FAIL_SYR);
                    }
                    else
                    {
                        g_syringe.status = SYRINGE_DRAIN_TO_ZERO;
                        g_syringe.isStatusSwitchStart = TRUE;
                        TRACE_INFO("\n Syringe status：SYRINGE_DRAIN_TO_ZERO");
                    }
                }
            }
            break;
        case SYRINGE_DRAIN_TO_ZERO:
            if (TRUE == g_syringe.isStatusSwitchStart)
            {
                g_syringe.isStatusSwitchStart = FALSE;
                if  (FALSE == StepperMotor_Start(&syringe->stepperMotor, FORWARD, 2100, TRUE, SyringeManager_MoveToZerosOffLimitHandle))
                {
                    TRACE_ERROR("\n Because no idle timer , the Syringe starts to fail.");
                    SyringeManager_Stop(syringe, RESULT_FAILED);
                    break;
                }
            }
            if (StepperMotor_IDLE == StepperMotor_GetStatus(&syringe->stepperMotor))
            {
                if (TRUE == syringe->isRequestStop)
                {
                    SyringeManager_Stop(syringe, RESULT_STOPPED);
                }
                else if(TRUE == StepperMotor_ReadDiagnostic(&syringe->stepperMotor))
                {
                    TRACE_ERROR("\n Stepper motor driver error");
                    SyringeManager_Stop(syringe, RESULT_DRIVER_ERROR);
                }
                else
                {
                    if (TRUE == SyringeManager_IsSensorBlocked())
                    {
                        TRACE_ERROR("\n Because error no find sensor, the Syringe fail. Photosensor is Blocked ");
                        SyringeManager_Stop(syringe, RESULT_MOVE_OUT_SENSOR_FAIL_SYR);
                    }
                    else
                    {
                        TRACE_INFO("\n Syringe reset finish.");
                        SyringeManager_Stop(syringe, RESULT_FINISHED);
                    }
                }
            }
            break;
        }
    }
}


