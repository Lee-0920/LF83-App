/*
 * DCMotorDriver.c
 *
 *  Created on: 2018年2月28日
 *      Author: LIANG
 */

#include "DCMotorDriver.h"

void DCMotorDriver_Init(DCMotorDriver *motorDriver)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(motorDriver->rcc, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_InitStructure.GPIO_Pin = motorDriver->pin;
    GPIO_Init(motorDriver->port, &GPIO_InitStructure);
    DCMotorDriver_Stop(motorDriver);
}

void DCMotorDriver_Start(DCMotorDriver *motorDriver)
{
    GPIO_SetBits(motorDriver->port, motorDriver->pin);
}

void DCMotorDriver_Stop(DCMotorDriver *motorDriver)
{
    GPIO_ResetBits(motorDriver->port, motorDriver->pin);
}
