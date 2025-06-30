/*
 * DCMotorDriver.h
 *
 *  Created on: 2018年2月28日
 *      Author: LIANG
 */

#ifndef SRC_DRIVER_LIQUIDDRIVER_DCMOTORDRIVER_H_
#define SRC_DRIVER_LIQUIDDRIVER_DCMOTORDRIVER_H_

#include "stm32f4xx.h"
#include "Common/Types.h"

typedef struct
{
    GPIO_TypeDef *port;
    Uint16 pin;
    uint32_t rcc;
}DCMotorDriver;

void DCMotorDriver_Init(DCMotorDriver *motorDriver);
void DCMotorDriver_Start(DCMotorDriver *motorDriver);
void DCMotorDriver_Stop(DCMotorDriver *motorDriver);

#endif /* SRC_DRIVER_LIQUIDDRIVER_DCMOTORDRIVER_H_ */
