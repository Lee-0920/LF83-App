/*
 * PositionSensorMap.c
 *
 *  Created on: 2018年3月6日
 *      Author: LIANG
 */
#include "PositionSensor.h"
#include "PositionSensorMap.h"

void PositionSensorMap_SyringeInit(Syringe *syringe)
{
    syringe[0].positionSensor.pin = GPIO_Pin_13;
    syringe[0].positionSensor.port = GPIOD;
    syringe[0].positionSensor.rcc = RCC_AHB1Periph_GPIOD;
    PositionSensor_Init(&syringe[0].positionSensor);
}

void PositionSensorMap_DisplacementMotorInit(DisplacementMotor *displacementMotor)
{
    displacementMotor[0].positionSensor.pin = GPIO_Pin_11;
    displacementMotor[0].positionSensor.port = GPIOD;
    displacementMotor[0].positionSensor.rcc = RCC_AHB1Periph_GPIOD;
    PositionSensor_Init(&displacementMotor[0].positionSensor);

    displacementMotor[1].positionSensor.pin = GPIO_Pin_12;
    displacementMotor[1].positionSensor.port = GPIOD;
    displacementMotor[1].positionSensor.rcc = RCC_AHB1Periph_GPIOD;
    PositionSensor_Init(&displacementMotor[1].positionSensor);
}

void PositionSensorMap_CollisionSensorInit(PositionSensor *positionSensor)
{
    positionSensor[0].pin = GPIO_Pin_6;
    positionSensor[0].port = GPIOC;
    positionSensor[0].rcc = RCC_AHB1Periph_GPIOC;
    PositionSensor_Init(&positionSensor[0]);

    positionSensor[1].pin = GPIO_Pin_7;
    positionSensor[1].port = GPIOC;
    positionSensor[1].rcc = RCC_AHB1Periph_GPIOC;
    PositionSensor_Init(&positionSensor[1]);
}

void PositionSensorMap_WaterCheckSensorInit(PositionSensor *positionSensor)
{
    positionSensor[0].pin = GPIO_Pin_14;
    positionSensor[0].port = GPIOD;
    positionSensor[0].rcc = RCC_AHB1Periph_GPIOD;
    PositionSensor_Init(&positionSensor[0]);

    positionSensor[1].pin = GPIO_Pin_15;
    positionSensor[1].port = GPIOD;
    positionSensor[1].rcc = RCC_AHB1Periph_GPIOD;
    PositionSensor_Init(&positionSensor[1]);
}

