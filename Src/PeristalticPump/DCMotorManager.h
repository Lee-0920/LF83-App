/*
 * DCMotorManager.h
 *
 *  Created on: 2018年3月9日
 *      Author: LIANG
 */

#ifndef SRC_PERISTALTICPUMP_DCMOTORMANAGER_H_
#define SRC_PERISTALTICPUMP_DCMOTORMANAGER_H_

#include "LiquidDriver/DCMotorDriver.h"
#include "FreeRTOS.h"
#include "timers.h"

#define DCMOTOR_TOTAL_PUMP 5

typedef struct
{
    DCMotorDriver driver;
    Bool isSendEvent;
    xTimerHandle timer;
    Bool isRunning;
    Uint32 startTime;
    Uint32 endTime;
    Uint8 number;
}DCMotor;

void DCMotorManager_Init();
void DCMotorManager_Restore();
Uint16 DCMotorManager_Start(Uint8 index, float volume);
Uint16 DCMotorManager_RequestStop(Uint8 index);
Uint16 DCMotorManager_GetTotalPumps();
float DCMotorManager_GetVolume(Uint8 index);
Bool DCMotorManager_SendEventOpen(Uint8 index);
Bool DCMotorManager_IsRunning(Uint8 index);

#endif /* SRC_PERISTALTICPUMP_DCMOTORMANAGER_H_ */
