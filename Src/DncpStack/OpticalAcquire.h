/*
 * OpticalAcquire.h
 *
 *  Created on: 2017年11月22日
 *      Author: LIANG
 */

#ifndef SRC_DNCPSTACK_OPTICALACQUIRE_H_
#define SRC_DNCPSTACK_OPTICALACQUIRE_H_

#include "LuipApi/OpticalAcquireInterface.h"
#include "DNCP/App/DscpDevice.h"

#define REF    0
#define MEA    1

void OpticalAcquire_SetSignalADNotifyPeriod(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_StartAcquirer(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_StopAcquirer(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_SetGain(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_GetGain(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_PowerOn(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_PowerOff(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquirer_StartStaticADControl(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquirer_StopStaticADControl(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquirer_GetStaticADControlParam(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquirer_SetStaticADControlParam(DscpDevice* dscp, Byte* data, Uint16 len);
void OpticalAcquire_IsADControlValid(DscpDevice* dscp, Byte* data, Uint16 len);

//命令入口，全局宏定义：每一条命令对应一个命令码和处理函数
#define CMD_TABLE_OPTICALACQUIRE \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_SET_SIGNAL_AD_NOTIFY_PERIOD, OpticalAcquire_SetSignalADNotifyPeriod), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_START_ACQUIRER, OpticalAcquire_StartAcquirer), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_STOP_ACQUIRER, OpticalAcquire_StopAcquirer), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_SET_GAIN, OpticalAcquire_SetGain), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_GET_GAIN, OpticalAcquire_GetGain),\
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_POWER_ON, OpticalAcquire_PowerOn), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_POWER_OFF, OpticalAcquire_PowerOff), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_START_STATIC_AD_CONTROL, OpticalAcquirer_StartStaticADControl), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_STOP_STATIC_AD_CONTROL, OpticalAcquirer_StopStaticADControl), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_GET_STATIC_AD_CONTROL_PARAM, OpticalAcquirer_GetStaticADControlParam), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_SET_STATIC_AD_CONTROL_PARAM, OpticalAcquirer_SetStaticADControlParam), \
        DSCP_CMD_ENTRY(DSCP_CMD_OAI_IS_STATIC_AD_CONTROL_VALID, OpticalAcquire_IsADControlValid)



#endif /* SRC_DNCPSTACK_OPTICALACQUIRE_H_ */
