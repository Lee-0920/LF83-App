/*
 * ThermostatDeviceMap.c
 *
 *  Created on: 2017年11月16日
 *      Author: LIANG
 */

#include "ThermostatDeviceMap.h"
#include "Driver/System.h"
#include <string.h>
#include "Tracer/Trace.h"

static Bool ThermostatDeviceMap_SetOutputWay1(ThermostatDeviceDriver *deviceDriver, float level);
static Bool ThermostatDeviceMap_SetOutputWay2(ThermostatDeviceDriver *deviceDriver, float level);

void ThermostatDeviceMap_Init(ThermostatDevice* device)
{
    //制冷设备
    //制冷片1
    device[0].maxDutyCycle = 1;
    device[0].setOutputWayFunc = ThermostatDeviceMap_SetOutputWay1;
    device[0].deviceDriver.mode = THERMOSTATDEVICEDRIVER_IO;
    device[0].deviceDriver.port = GPIOA;
    device[0].deviceDriver.pin = GPIO_Pin_1;
    device[0].deviceDriver.gpioRcc = RCC_AHB1Periph_GPIOA;
    device[0].deviceDriver.modeConfig.IOConfig.open = Bit_SET;
    device[0].deviceDriver.modeConfig.IOConfig.close = Bit_RESET;
    ThermostatDevice_Init(&device[0]);
    //制冷片2
    device[1].maxDutyCycle = 1;
    device[1].setOutputWayFunc = ThermostatDeviceMap_SetOutputWay1;
    device[1].deviceDriver.mode = THERMOSTATDEVICEDRIVER_IO;
    device[1].deviceDriver.port = GPIOA;
    device[1].deviceDriver.pin = GPIO_Pin_2;
    device[1].deviceDriver.gpioRcc = RCC_AHB1Periph_GPIOA;
    device[1].deviceDriver.modeConfig.IOConfig.open = Bit_SET;
    device[1].deviceDriver.modeConfig.IOConfig.close = Bit_RESET;
    ThermostatDevice_Init(&device[1]);

    //冷凝风扇0
    device[2].maxDutyCycle = 1;
    device[2].setOutputWayFunc = ThermostatDeviceMap_SetOutputWay1;
    device[2].deviceDriver.mode = THERMOSTATDEVICEDRIVER_IO;
    device[2].deviceDriver.port = GPIOA;
    device[2].deviceDriver.pin = GPIO_Pin_3;
    device[2].deviceDriver.gpioRcc = RCC_AHB1Periph_GPIOA;
    device[2].deviceDriver.modeConfig.IOConfig.open = Bit_SET;
    device[2].deviceDriver.modeConfig.IOConfig.close = Bit_RESET;
    ThermostatDevice_Init(&device[2]);
    //冷凝风扇1
    device[3].maxDutyCycle = 1;
    device[3].setOutputWayFunc = ThermostatDeviceMap_SetOutputWay1;
    device[3].deviceDriver.mode = THERMOSTATDEVICEDRIVER_IO;
    device[3].deviceDriver.port = GPIOA;
    device[3].deviceDriver.pin = GPIO_Pin_4;
    device[3].deviceDriver.gpioRcc = RCC_AHB1Periph_GPIOA;
    device[3].deviceDriver.modeConfig.IOConfig.open = Bit_SET;
    device[3].deviceDriver.modeConfig.IOConfig.close = Bit_RESET;
    ThermostatDevice_Init(&device[3]);

    //机箱风扇IO模式
    device[4].maxDutyCycle = 1;
    device[4].setOutputWayFunc = ThermostatDeviceMap_SetOutputWay1;
    device[4].deviceDriver.mode = THERMOSTATDEVICEDRIVER_IO;
    device[4].deviceDriver.port = GPIOB;
    device[4].deviceDriver.pin = GPIO_Pin_0;
    device[4].deviceDriver.gpioRcc = RCC_AHB1Periph_GPIOB;
    device[4].deviceDriver.modeConfig.IOConfig.open = Bit_SET;
    device[4].deviceDriver.modeConfig.IOConfig.close = Bit_RESET;
    ThermostatDevice_Init(&device[4]);

    //加热设备
    //制热片
    device[5].maxDutyCycle = 1;
    device[5].setOutputWayFunc = ThermostatDeviceMap_SetOutputWay1;
    device[5].deviceDriver.mode = THERMOSTATDEVICEDRIVER_IO;
    device[5].deviceDriver.port = GPIOB;
    device[5].deviceDriver.pin = GPIO_Pin_0;
    device[5].deviceDriver.gpioRcc = RCC_AHB1Periph_GPIOB;
    device[5].deviceDriver.modeConfig.IOConfig.open = Bit_SET;
    device[5].deviceDriver.modeConfig.IOConfig.close = Bit_RESET;
    ThermostatDevice_Init(&device[5]);
}

static Bool ThermostatDeviceMap_SetOutputWay1(ThermostatDeviceDriver *deviceDriver, float level)
{
    TRACE_CODE("\n Output way 1");
    return ThermostatDeviceDriver_SetOutput(deviceDriver, level);
}

static Bool ThermostatDeviceMap_SetOutputWay2(ThermostatDeviceDriver *deviceDriver, float level)
{
    TRACE_CODE("\n Output way 2");
    if (0 != level)
    {
        level = 0.5 * level + 0.5;
        if (level < 0.75)
        {
            ThermostatDeviceDriver_SetOutput(deviceDriver, 1);
            System_Delay(200);
        }
    }
    return ThermostatDeviceDriver_SetOutput(deviceDriver, level);
}

char* ThermostatDeviceMap_GetName(Uint8 index)
{
    static char name[35] = "";
    memset(name, 0, sizeof(name));
    switch(index)
    {
    case MEAROOM_COOLER:
        strcpy(name, "MeasureRoomCooler");
        break;
    case MEAROOM_HEATER:
        strcpy(name, "MeasureRoomHeater");
        break;
    case BACTROOM_COOLER:
        strcpy(name, "BacteriaRoomCooler");
        break;
    case CONDENSATOR_FAN_0:
        strcpy(name, "CondensatorFan0");
        break;
    case CONDENSATOR_FAN_1:
        strcpy(name, "CondensatorFan1");
        break;
    case BOX_FAN:
        strcpy(name, "BoxFan");
        break;
    default:
        strcpy(name, "NULL");
        break;
    }
    return name;
}
