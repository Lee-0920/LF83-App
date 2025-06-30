/*
 * StepperMotorDriver.c
 *
 *  Created on: 2016年5月30日
 *      Author: Administrator
 */
#include "StepperMotorDriver.h"

void StepperMotorDriver_Init(StepperMotorDriver *stepperMotorDriver)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(
            stepperMotorDriver->rccClock | stepperMotorDriver->rccDir | stepperMotorDriver->rccDiag,
            ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    ///GPIO_InitStructure.GPIO_Pin = stepperMotorDriver->pinEnable;
    ///GPIO_Init(stepperMotorDriver->portEnable, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = stepperMotorDriver->pinClock;
    GPIO_Init(stepperMotorDriver->portClock, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = stepperMotorDriver->pinDir;
    GPIO_Init(stepperMotorDriver->portDir, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Pin = stepperMotorDriver->pinDiag;
    GPIO_Init(stepperMotorDriver->portDiag, &GPIO_InitStructure);

    stepperMotorDriver->forwardLevel = Bit_RESET;
}

void StepperMotorDriver_Enable(StepperMotorDriver *stepperMotorDriver)
{
    ///GPIO_ResetBits(stepperMotorDriver->portEnable, stepperMotorDriver->pinEnable);
}

void StepperMotorDriver_Disable(StepperMotorDriver *stepperMotorDriver)
{
    ///GPIO_SetBits(stepperMotorDriver->portEnable, stepperMotorDriver->pinEnable);
}

void StepperMotorDriver_SetDirection(StepperMotorDriver *stepperMotorDriver, Direction dir)
{
    if (BACKWARD == dir)
    {
        BitAction bitVal = Bit_RESET;
        if (stepperMotorDriver->forwardLevel == Bit_RESET)
        {
            bitVal = Bit_SET;
        }
        GPIO_WriteBit(stepperMotorDriver->portDir, stepperMotorDriver->pinDir, bitVal);
    }
    else
    {
        GPIO_WriteBit(stepperMotorDriver->portDir, stepperMotorDriver->pinDir, stepperMotorDriver->forwardLevel);
    }
}

void StepperMotorDriver_PullHigh(StepperMotorDriver *stepperMotorDriver)
{
    GPIO_SetBits(stepperMotorDriver->portClock, stepperMotorDriver->pinClock);
}

void StepperMotorDriver_PullLow(StepperMotorDriver *stepperMotorDriver)
{
    GPIO_ResetBits(stepperMotorDriver->portClock, stepperMotorDriver->pinClock);
}

Bool StepperMotorDriver_ReadDiagnostic(StepperMotorDriver *stepperMotorDriver)
{
    return GPIO_ReadInputDataBit(stepperMotorDriver->portDiag, stepperMotorDriver->pinDiag);
}

void StepperMotorDriver_SetForwardLevel(StepperMotorDriver *stepperMotorDriver, BitAction bitVal)
{
    stepperMotorDriver->forwardLevel = bitVal;
}
