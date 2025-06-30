/*
 * PMTControl.h
 *
 *  Created on: 2019年11月4日
 *      Author: Administrator
 */

#ifndef SRC_OPTICALCONTROL_PMTCONTROL_H_
#define SRC_OPTICALCONTROL_PMTCONTROL_H_

#include "stm32f4xx.h"
#include "Common/Types.h"

typedef struct
{
    Uint32 refValue;
    Uint32 meaValue;
}PmtResult;

typedef enum
{
        PMT_IDLE  = 0,
        PMT_BUSY = 1,
}PMT_STATUS;

void PMTControl_Init(void);
Bool PMTControl_StartAcquire(Uint32 timeout, Bool isSendEvent);
void PMTControl_StopAcquire(Bool isSendEvent);
Bool PMTControl_GetNewResult(PmtResult* result);
Uint8 PMTControl_GetStatus(void);
void PMTControl_PowerOn(void);
void PMTControl_PowerOff(void);
Bool PMTControl_PowerStatus(void);

#endif /* SRC_OPTICALCONTROL_PMTCONTROL_H_ */
