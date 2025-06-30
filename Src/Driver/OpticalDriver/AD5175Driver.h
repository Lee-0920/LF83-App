/*
 * AD5175Driver.h
 *
 *  Created on: 2018年11月19日
 *      Author: Administrator
 */

#ifndef SRC_DRIVER_OPTICALDRIVER_AD5175DRIVER_H_
#define SRC_DRIVER_OPTICALDRIVER_AD5175DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include "Driver/System.h"

#define AD5175_MAX_VALUE    0x3FF
#define AD5175_MIN_VALUE    0x0
#define AD5175_ADDR_0   0x2C        //Vdd
#define AD5175_ADDR_1   0x2F        //GND
#define AD5175_ADDR_2   0x2E        //NC

typedef struct
{
    GPIO_TypeDef *portSCL;
    Uint16 pinSCL;
    uint32_t rccSCL;
    GPIO_TypeDef *portSDA;
    Uint16 pinSDA;
    uint32_t rccSDA;
} AD5175Driver;

/**
 * @brief AD5175驱动
 * @details 使用I2C方式实现数字电位器AD5175读写控制
 */
void AD5175_Init(AD5175Driver* ad5175);
Uint16 AD5175_ReadWord(AD5175Driver* ad5175, Uint8 addr);
Bool AD5175_WriteWord(AD5175Driver* ad5175, Uint8 addr, Uint16 word);
Bool AD5175_WriteWords(AD5175Driver* ad5175, Uint8 addr, Uint16* buffer, Uint8 len);
Uint16 AD5175_ReadRDAC(AD5175Driver* ad5175, Uint8 addr);
Bool AD5175_WriteRDAC(AD5175Driver* ad5175, Uint8 addr, Uint16 value);
Bool AD5175_SaveRDACTo50TP(AD5175Driver* ad5175, Uint8 addr);

#ifdef __cplusplus
}
#endif

#endif /* SRC_DRIVER_OPTICALDRIVER_AD5175DRIVER_H_ */
