/*
 * StepperMotorMap.c
 *
 *  Created on: 2016年5月30日
 *      Author: Administrator
 */

#include "stm32f4xx.h"
#include "StepperMotorDriver.h"
#include "StepperMotorMap.h"

void StepperMotorMap_PeristalticPumpInit(PeristalticPump *peristalticPump)
{
    peristalticPump[0].stepperMotor.driver.pinClock = GPIO_Pin_8;
    peristalticPump[0].stepperMotor.driver.portClock = GPIOE;
    peristalticPump[0].stepperMotor.driver.rccClock = RCC_AHB1Periph_GPIOE;

    peristalticPump[0].stepperMotor.driver.pinDir = GPIO_Pin_9;
    peristalticPump[0].stepperMotor.driver.portDir = GPIOE;
    peristalticPump[0].stepperMotor.driver.rccDir = RCC_AHB1Periph_GPIOE;

    ///peristalticPump[0].stepperMotor.driver.pinEnable = GPIO_Pin_11;
    ///peristalticPump[0].stepperMotor.driver.portEnable = GPIOA;
    ///peristalticPump[0].stepperMotor.driver.rccEnable = RCC_AHB1Periph_GPIOA;

    peristalticPump[0].stepperMotor.driver.pinDiag = GPIO_Pin_10;
    peristalticPump[0].stepperMotor.driver.portDiag = GPIOE;
    peristalticPump[0].stepperMotor.driver.rccDiag = RCC_AHB1Periph_GPIOE;


    StepperMotorDriver_Init(&peristalticPump[0].stepperMotor.driver);
    StepperMotorDriver_PullLow(&peristalticPump[0].stepperMotor.driver);
    StepperMotorDriver_Disable(&peristalticPump[0].stepperMotor.driver);
    StepperMotorDriver_SetForwardLevel(&peristalticPump[0].stepperMotor.driver, Bit_RESET);
}

void StepperMotorMap_SyringeInit(Syringe *syringe)
{
    syringe[0].stepperMotor.driver.pinClock = GPIO_Pin_11;
    syringe[0].stepperMotor.driver.portClock = GPIOA;
    syringe[0].stepperMotor.driver.rccClock = RCC_AHB1Periph_GPIOA;

    syringe[0].stepperMotor.driver.pinDir = GPIO_Pin_12;
    syringe[0].stepperMotor.driver.portDir = GPIOA;
    syringe[0].stepperMotor.driver.rccDir = RCC_AHB1Periph_GPIOA;

    ///syringe[0].stepperMotor.driver.pinEnable = GPIO_Pin_8;
    ///syringe[0].stepperMotor.driver.portEnable = GPIOC;
    ///syringe[0].stepperMotor.driver.rccEnable = RCC_AHB1Periph_GPIOC;

    syringe[0].stepperMotor.driver.pinDiag = GPIO_Pin_13;
    syringe[0].stepperMotor.driver.portDiag = GPIOC;
    syringe[0].stepperMotor.driver.rccDiag = RCC_AHB1Periph_GPIOC;

    StepperMotorDriver_Init(&syringe[0].stepperMotor.driver);
    StepperMotorDriver_PullLow(&syringe[0].stepperMotor.driver);
    StepperMotorDriver_Disable(&syringe[0].stepperMotor.driver);
    StepperMotorDriver_SetForwardLevel(&syringe[0].stepperMotor.driver, Bit_SET);
}

void StepperMotorMap_DisplacementMotorInit(DisplacementMotor *displacementMotor)
{
    displacementMotor[0].stepperMotor.driver.pinClock = GPIO_Pin_6;
    displacementMotor[0].stepperMotor.driver.portClock = GPIOA;
    displacementMotor[0].stepperMotor.driver.rccClock = RCC_AHB1Periph_GPIOA;

    displacementMotor[0].stepperMotor.driver.pinDir = GPIO_Pin_7;
    displacementMotor[0].stepperMotor.driver.portDir = GPIOA;
    displacementMotor[0].stepperMotor.driver.rccDir = RCC_AHB1Periph_GPIOA;

    displacementMotor[0].stepperMotor.driver.pinDiag = GPIO_Pin_7;
    displacementMotor[0].stepperMotor.driver.portDiag = GPIOE;
    displacementMotor[0].stepperMotor.driver.rccDiag = RCC_AHB1Periph_GPIOE;

    ///displacementMotor[0].stepperMotor.driver.pinEnable = GPIO_Pin_0;
    ///displacementMotor[0].stepperMotor.driver.portEnable = GPIOC;
    ///displacementMotor[0].stepperMotor.driver.rccEnable = RCC_AHB1Periph_GPIOC;

    StepperMotorDriver_Init(&displacementMotor[0].stepperMotor.driver);
    StepperMotorDriver_PullLow(&displacementMotor[0].stepperMotor.driver);
    //StepperMotorDriver_Disable(&displacementMotor[0].stepperMotor.driver);
    StepperMotorDriver_Enable(&displacementMotor[0].stepperMotor.driver);
    StepperMotorDriver_SetForwardLevel(&displacementMotor[0].stepperMotor.driver, Bit_SET);

    displacementMotor[1].stepperMotor.driver.pinClock = GPIO_Pin_8;
    displacementMotor[1].stepperMotor.driver.portClock = GPIOC;
    displacementMotor[1].stepperMotor.driver.rccClock = RCC_AHB1Periph_GPIOC;

    displacementMotor[1].stepperMotor.driver.pinDir = GPIO_Pin_9;
    displacementMotor[1].stepperMotor.driver.portDir = GPIOC;
    displacementMotor[1].stepperMotor.driver.rccDir = RCC_AHB1Periph_GPIOC;

    ///displacementMotor[1].stepperMotor.driver.pinEnable = GPIO_Pin_2;
    ///displacementMotor[1].stepperMotor.driver.portEnable = GPIOE;
    ///displacementMotor[1].stepperMotor.driver.rccEnable = RCC_AHB1Periph_GPIOE;

    displacementMotor[1].stepperMotor.driver.pinDiag = GPIO_Pin_2;
    displacementMotor[1].stepperMotor.driver.portDiag = GPIOE;
    displacementMotor[1].stepperMotor.driver.rccDiag = RCC_AHB1Periph_GPIOE;

    StepperMotorDriver_Init(&displacementMotor[1].stepperMotor.driver);
    StepperMotorDriver_PullLow(&displacementMotor[1].stepperMotor.driver);
    //StepperMotorDriver_Disable(&displacementMotor[1].stepperMotor.driver);
    StepperMotorDriver_Enable(&displacementMotor[1].stepperMotor.driver);
    StepperMotorDriver_SetForwardLevel(&displacementMotor[1].stepperMotor.driver, Bit_SET);
}
