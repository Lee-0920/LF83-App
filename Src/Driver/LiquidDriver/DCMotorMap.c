/*
 * DCMotorMap.c
 *
 *  Created on: 2018年2月28日
 *      Author: LIANG
 */

#include "DCMotorMap.h"

void DCMotorMap_Init(DCMotor *motor)
{
    motor[0].driver.pin = GPIO_Pin_6;
    motor[0].driver.port = GPIOB;
    motor[0].driver.rcc = RCC_AHB1Periph_GPIOB;
    DCMotorDriver_Init(&motor[0].driver);

    motor[1].driver.pin = GPIO_Pin_7;
    motor[1].driver.port = GPIOB;
    motor[1].driver.rcc = RCC_AHB1Periph_GPIOB;
    DCMotorDriver_Init(&motor[1].driver);

    motor[2].driver.pin = GPIO_Pin_0;
    motor[2].driver.port = GPIOE;
    motor[2].driver.rcc = RCC_AHB1Periph_GPIOE;
    DCMotorDriver_Init(&motor[2].driver);

    motor[3].driver.pin = GPIO_Pin_8;
    motor[3].driver.port = GPIOD;
    motor[3].driver.rcc = RCC_AHB1Periph_GPIOD;
    DCMotorDriver_Init(&motor[3].driver);

    motor[4].driver.pin = GPIO_Pin_9;
    motor[4].driver.port = GPIOD;
    motor[4].driver.rcc = RCC_AHB1Periph_GPIOD;
    DCMotorDriver_Init(&motor[4].driver);

//    motor[5].driver.pin = GPIO_Pin_10;
//    motor[5].driver.port = GPIOD;
//    motor[5].driver.rcc = RCC_AHB1Periph_GPIOD;
//    DCMotorDriver_Init(&motor[5].driver);
}
