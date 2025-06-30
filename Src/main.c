/**
 * @file main.c
 * @brief 
 * @details
 *
 * @version 1.0.0
 * @author xingfan
 * @date 2016-4-28
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "System.h"
#include "console/Console.h"
#include "CmdLine.h"
#include "DncpStack.h"
#include "DeviceInfo.h"
#include "DeviceIndicatorLED.h"
#include "Watchdog.h"
#include "DeviceStatus.h"
#include "DeviceUpdate/UpdateHandle.h"
#include "SolenoidValve/ValveManager.h"
#include "LiquidDriver/StepperMotorTimer.h"
#include "PeristalticPump/PumpEventScheduler.h"
#include "PeristalticPump/PeristalticPumpManager.h"
#include "PeristalticPump/SyringeManager.h"
#include "PeristalticPump/DisplacementMotorManager.h"
#include "PeristalticPump/DCMotorManager.h"
#include "TemperatureControl/TempCollecterManager.h"
#include "TemperatureControl/ThermostatDeviceManager.h"
#include "TemperatureControl/ThermostatManager.h"
#include "OpticalControl/PMTControl.h"
#include "OpticalControl/StaticADControl.h"
#include "PeristalticPump/TMCConfig.h"

int main(void)
{
    System_Init();
    DeviceIndicatorLED_Init();
    Watchdog_Init();
    // 功能模块初始化
    Console_Init();
    CmdLine_Init();

    DncpStack_Init();

    ValveManager_Init();
    StepperMotorTimer_Init();
    PumpEventScheduler_Init();
    PeristalticPumpManager_Init();
    SyringeManager_Init();
    DisplacementMotor_Init();
    DCMotorManager_Init();
    TMCConfig_Init();

    TempCollecterManager_Init();
    ThermostatDeviceManager_Init();
    ThermostatManager_Init();

    PMTControl_Init();
    StaticADControl_Init();

    DeviceInfo_Init();
 	UpdateHandle_Init();
    DeviceStatus_ReportResetEvent(DEVICE_RESET_POWER_ON); // 报告复位事件

    vTaskStartScheduler();

    /* Infinite loop */
    while (1)
    {
    }
}
#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/**
 * @}
 */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
