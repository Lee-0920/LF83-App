/*
 * PMTDriver.c
 *
 *  Created on: 2019年11月4日
 *      Author: Administrator
 */
#include "PMTDriver.h"
#include "Common/Types.h"
#include "SystemConfig.h"
#include "Tracer/Trace.h"
#include "FreeRTOS.h"
#include "task.h"

/*************PMT_POWER******************/
#define PMT_POWER_PLUS_PIN              GPIO_Pin_4
#define PMT_POWER_PLUS_PORT             GPIOC
#define PMT_POWER_PLUS_RCC              RCC_AHB1Periph_GPIOC
#define PMT_POWER_PLUS_ON()                GPIO_SetBits(PMT_POWER_PLUS_PORT, PMT_POWER_PLUS_PIN)
#define PMT_POWER_PLUS_OFF()              GPIO_ResetBits(PMT_POWER_PLUS_PORT, PMT_POWER_PLUS_PIN)

#define PMT_POWER_MINUS_PIN              GPIO_Pin_5
#define PMT_POWER_MINUS_PORT             GPIOC
#define PMT_POWER_MINUS_RCC              RCC_AHB1Periph_GPIOC
#define PMT_POWER_MINUS_ON()                GPIO_SetBits(PMT_POWER_MINUS_PORT, PMT_POWER_MINUS_PIN)
#define PMT_POWER_MINUS_OFF()              GPIO_ResetBits(PMT_POWER_MINUS_PORT, PMT_POWER_MINUS_PIN)

/*************PMT_REF******************/
//输入引脚
#define PMT_REF_COUNTER_IN_PIN              GPIO_Pin_5
#define PMT_REF_COUNTER_PINSOURCE    GPIO_PinSource5
#define PMT_REF_COUNTER_IN_PORT             GPIOA
#define PMT_REF_COUNTER_IN_RCC              RCC_AHB1Periph_GPIOA
#define PMT_REF_COUNTER_IN_AF_TIM              GPIO_AF_TIM2

// 定时器
#define PMT_REF_COUNTER_TIMER_RCC                     RCC_APB1Periph_TIM2
#define PMT_REF_COUNTER_TIMER_IRQn                    TIM2_IRQn
#define PMT_REF_COUNTER_IRQHANDLER                  TIM2_IRQHandler
#define PMT_REF_COUNTER_TIMER                               TIM2
#define PMT_REF_COUNTER_PERIOD            0xFFFFFFFF

/*************PMT_MEA******************/
//输入引脚
#define PMT_MEA_COUNTER_IN_PIN              GPIO_Pin_0
#define PMT_MEA_COUNTER_PINSOURCE    GPIO_PinSource0
#define PMT_MEA_COUNTER_IN_PORT             GPIOA
#define PMT_MEA_COUNTER_IN_RCC              RCC_AHB1Periph_GPIOA
#define PMT_MEA_COUNTER_IN_AF_TIM              GPIO_AF_TIM5

// 定时器
#define PMT_MEA_COUNTER_TIMER_RCC                     RCC_APB1Periph_TIM5
#define PMT_MEA_COUNTER_TIMER_IRQn                    TIM5_IRQn
#define PMT_MEA_COUNTER_IRQHANDLER                  TIM5_IRQHandler
#define PMT_MEA_COUNTER_TIMER                               TIM5
#define PMT_MEA_COUNTER_PERIOD            0xFFFFFFFF

__IO Uint32 s_currentCountRef = 0;
__IO Uint32 s_currentCountMea = 0;

/**
 * @brief PMT驱动初始化
 */
void PMTDriver_Init(void)
{
    PMTDriver_InitRef();
    PMTDriver_InitMea();
    PMTDriver_PowerPinInit();

    s_currentCountRef = 0;
    s_currentCountMea = 0;
}

/**
 * @brief 电源控制引脚初始化
 */
void PMTDriver_PowerPinInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(PMT_POWER_PLUS_RCC | PMT_POWER_MINUS_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_InitStructure.GPIO_Pin = PMT_POWER_PLUS_PIN;
    GPIO_Init(PMT_POWER_PLUS_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PMT_POWER_MINUS_PIN;
    GPIO_Init(PMT_POWER_MINUS_PORT, &GPIO_InitStructure);

    PMT_POWER_PLUS_OFF();
    PMT_POWER_MINUS_OFF();
}

/**
 * @brief 电源使能
 */
void PMTDriver_PowerOn(void)
{
    PMT_POWER_PLUS_ON();
    PMT_POWER_MINUS_ON();
}

/**
 * @brief 电源关闭
 */
void PMTDriver_PowerOff(void)
{
    PMT_POWER_PLUS_OFF();
    PMT_POWER_MINUS_OFF();
}

/**
 * @brief PMT_REF驱动初始化
 */
void PMTDriver_InitRef(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel                   = PMT_REF_COUNTER_TIMER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PMT_COUNTER_TIMER_IQR_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //时钟配置
    RCC_AHB1PeriphClockCmd(PMT_REF_COUNTER_IN_RCC , ENABLE);
    RCC_APB1PeriphClockCmd(PMT_REF_COUNTER_TIMER_RCC , ENABLE);

    //IO配置
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin = PMT_REF_COUNTER_IN_PIN;
    GPIO_Init(PMT_REF_COUNTER_IN_PORT, &GPIO_InitStructure);
    GPIO_PinAFConfig(PMT_REF_COUNTER_IN_PORT, PMT_REF_COUNTER_PINSOURCE, PMT_REF_COUNTER_IN_AF_TIM);

    //定时器配置
    TIM_DeInit(PMT_REF_COUNTER_TIMER);
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_Period = PMT_REF_COUNTER_PERIOD;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(PMT_REF_COUNTER_TIMER, &TIM_TimeBaseStructure);
    TIM_TIxExternalClockConfig(PMT_REF_COUNTER_TIMER, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Rising, 0);
    TIM_ClearFlag(PMT_REF_COUNTER_TIMER, TIM_IT_Update);
    TIM_ITConfig(PMT_REF_COUNTER_TIMER, TIM_IT_Update, ENABLE);  // 使能计数中断

    TIM_Cmd(PMT_REF_COUNTER_TIMER, DISABLE);
}

/**
 * @brief PMT_MEA驱动初始化
 */
void PMTDriver_InitMea(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel                   = PMT_MEA_COUNTER_TIMER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PMT_COUNTER_TIMER_IQR_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //时钟配置
    RCC_AHB1PeriphClockCmd(PMT_MEA_COUNTER_IN_RCC , ENABLE);
    RCC_APB1PeriphClockCmd(PMT_MEA_COUNTER_TIMER_RCC , ENABLE);

    //IO配置
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin = PMT_MEA_COUNTER_IN_PIN;
    GPIO_Init(PMT_MEA_COUNTER_IN_PORT, &GPIO_InitStructure);
    GPIO_PinAFConfig(PMT_MEA_COUNTER_IN_PORT, PMT_MEA_COUNTER_PINSOURCE, PMT_MEA_COUNTER_IN_AF_TIM);

    //定时器配置
    TIM_DeInit(PMT_MEA_COUNTER_TIMER);
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_Period = PMT_MEA_COUNTER_PERIOD;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(PMT_MEA_COUNTER_TIMER, &TIM_TimeBaseStructure);
    TIM_TIxExternalClockConfig(PMT_MEA_COUNTER_TIMER, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Rising, 0);
    TIM_ClearFlag(PMT_MEA_COUNTER_TIMER, TIM_IT_Update);
    TIM_ITConfig(PMT_MEA_COUNTER_TIMER, TIM_IT_Update, ENABLE);  // 使能计数中断

    TIM_Cmd(PMT_MEA_COUNTER_TIMER, DISABLE);
}


/**
 * @brief PMT_REF驱动使能
 */
void PMTDriver_EnableRef(void)
{
    TIM_SetCounter(PMT_REF_COUNTER_TIMER, 0);
    TIM_Cmd(PMT_REF_COUNTER_TIMER, ENABLE);

    s_currentCountRef = 0;
}

/**
 * @brief PMT_REF驱动失能
 */
void PMTDriver_DisableRef(void)
{
    // TIM_SetCounter(PMT_REF_COUNTER_TIMER, 0);
    TIM_Cmd(PMT_REF_COUNTER_TIMER, DISABLE);

    // s_currentCountRef = 0;
}

/**
 * @brief PMT_REF驱动读取计数器
 */
Uint32 PMTDriver_ReadCounterRef(void)
{
    Uint32 currentCountRef = 0;

    s_currentCountRef += TIM_GetCounter(PMT_REF_COUNTER_TIMER);
    currentCountRef = s_currentCountRef;
    s_currentCountRef = 0;

    TIM_SetCounter(PMT_REF_COUNTER_TIMER, 0);

    return currentCountRef;
}

/**
 * @brief PMT_REF计数器中断
 */
void PMT_REF_COUNTER_IRQHANDLER(void)
{
    /*
    if(TIM_GetITStatus(PMT_REF_COUNTER_TIMER, TIM_IT_Update) == SET)
    {
        TIM_SetCounter(PMT_REF_COUNTER_TIMER, 0);

        s_currentCountRef += PMT_REF_COUNTER_PERIOD;
    }

    TIM_ClearITPendingBit(PMT_REF_COUNTER_TIMER, TIM_IT_Update);
    */

    TIM_ClearITPendingBit(PMT_REF_COUNTER_TIMER, TIM_IT_Update);

    TIM_SetCounter(PMT_REF_COUNTER_TIMER, 0);

    s_currentCountRef += PMT_REF_COUNTER_PERIOD;
}

/**
 * @brief PMT_MEA驱动使能
 */
void PMTDriver_EnableMea(void)
{
    TIM_SetCounter(PMT_MEA_COUNTER_TIMER, 0);
    TIM_Cmd(PMT_MEA_COUNTER_TIMER, ENABLE);

    s_currentCountMea = 0;
}

/**
 * @brief PMT_MEA驱动失能
 */
void PMTDriver_DisableMea(void)
{
    // TIM_SetCounter(PMT_MEA_COUNTER_TIMER, 0);
    TIM_Cmd(PMT_MEA_COUNTER_TIMER, DISABLE);

    // s_currentCountMea = 0;
}

/**
 * @brief PMT_MEA驱动读取计数器
 */
Uint32 PMTDriver_ReadCounterMea(void)
{
    Uint32 currentCountMea = 0;

    s_currentCountMea += TIM_GetCounter(PMT_MEA_COUNTER_TIMER);
    currentCountMea = s_currentCountMea;
    s_currentCountMea = 0;
    TIM_SetCounter(PMT_MEA_COUNTER_TIMER, 0);

    return currentCountMea;
}

/**
 * @brief PMT_MEA计数器中断
 */
void PMT_MEA_COUNTER_IRQHANDLER(void)
{
    /*
    if(TIM_GetITStatus(PMT_MEA_COUNTER_TIMER, TIM_IT_Update) == SET)
    {
        TIM_SetCounter(PMT_MEA_COUNTER_TIMER, 0);
        s_currentCountMea += PMT_MEA_COUNTER_PERIOD;
    }

    TIM_ClearITPendingBit(PMT_MEA_COUNTER_TIMER, TIM_IT_Update);
    */

    TIM_ClearITPendingBit(PMT_MEA_COUNTER_TIMER, TIM_IT_Update);

    TIM_SetCounter(PMT_MEA_COUNTER_TIMER, 0);
    s_currentCountMea += PMT_MEA_COUNTER_PERIOD;
}
