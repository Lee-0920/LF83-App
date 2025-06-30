/*
 * ValveMap.c
 *
 *  Created on: 2016年6月7日
 *      Author: Administrator
 */

#include "stm32f4xx.h"
#include "ValveMap.h"
#include "SolenoidValve/ValveManager.h"

void ValveMap_Init(Valve *valve)
{
    Uint8 i;

    valve[0].pin = GPIO_Pin_12;
    valve[0].port = GPIOC;
    valve[0].rcc = RCC_AHB1Periph_GPIOC;
    ValveDriver_Init(&valve[0]);

    valve[1].pin = GPIO_Pin_0;
    valve[1].port = GPIOD;
    valve[1].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[1]);

    valve[2].pin = GPIO_Pin_1;
    valve[2].port = GPIOD;
    valve[2].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[2]);

    valve[3].pin = GPIO_Pin_2;
    valve[3].port= GPIOD;
    valve[3].rcc =  RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[3]);

    valve[4].pin = GPIO_Pin_3;
    valve[4].port = GPIOD;
    valve[4].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[4]);

    valve[5].pin = GPIO_Pin_4;
    valve[5].port = GPIOD;
    valve[5].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[5]);

    valve[6].pin = GPIO_Pin_5;
    valve[6].port = GPIOD;
    valve[6].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[6]);

    valve[7].pin = GPIO_Pin_6;
    valve[7].port = GPIOD;
    valve[7].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[7]);

    valve[8].pin = GPIO_Pin_7;
    valve[8].port = GPIOD;
    valve[8].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[8]);

    valve[9].pin = GPIO_Pin_5;
    valve[9].port = GPIOB;
    valve[9].rcc = RCC_AHB1Periph_GPIOB;
    ValveDriver_Init(&valve[9]);

    for(i = 0; i < SOLENOIDVALVECONF_TOTALVAlVES; i++)
    {
        ValveDriver_Control(&valve[i], VAlVE_CLOSE);
    }
}


