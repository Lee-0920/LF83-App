/*
 * TempCollecterMap.c
 *
 *  Created on: 2017年11月20日
 *      Author: LIANG
 */

#include "TempCollecterMap.h"
#include "Tracer/Trace.h"
#include "Driver/System.h"
#include <string.h>

#define TEMPADCOLLECT_AD_MAX  (4095)    // 24位AD
#define TEMPADCOLLECT_V_REF  (2.5)       // 单位(V)

//温度校准系统初始化参数
const static TempCalibrateParam s_kTempCalculateParam1 =
{ .negativeInput = 1.5000, .vref = 2.4901, .vcal = 0, };

const static TempCalibrateParam s_kTempCalculateParam2 =
{ .negativeInput = 1.9905, .vref = 2.5001, .vcal = 0, };

static double TempCollecterMap_GetResistanceValue1(TempCalibrateParam *tempCalibrateParam, Uint16 ad);
static double TempCollecterMap_GetResistanceValue2(TempCalibrateParam *tempCalibrateParam, Uint16 ad);

void TempCollecterMap_Init(TempCollecter *tempCollecter)
{
    tempCollecter[MEAROOM_TEMP].getResistanceValueFunc = TempCollecterMap_GetResistanceValue1;
    TempCollecter_SetNumber(&tempCollecter[MEAROOM_TEMP], MEAROOM_TEMP);
    TempCollecter_Init(&tempCollecter[MEAROOM_TEMP], s_kTempCalculateParam1);

    tempCollecter[BACTROOM_TEMP].getResistanceValueFunc = TempCollecterMap_GetResistanceValue1;
    TempCollecter_SetNumber(&tempCollecter[BACTROOM_TEMP], BACTROOM_TEMP);
    TempCollecter_Init(&tempCollecter[BACTROOM_TEMP], s_kTempCalculateParam1);

    tempCollecter[MEAROOM_OUT_TEMP].getResistanceValueFunc = TempCollecterMap_GetResistanceValue1;
    TempCollecter_SetNumber(&tempCollecter[MEAROOM_OUT_TEMP], MEAROOM_OUT_TEMP);
    TempCollecter_Init(&tempCollecter[MEAROOM_OUT_TEMP], s_kTempCalculateParam1);

    tempCollecter[BACTROOM_OUT_TEMP].getResistanceValueFunc = TempCollecterMap_GetResistanceValue1;
    TempCollecter_SetNumber(&tempCollecter[BACTROOM_OUT_TEMP], BACTROOM_OUT_TEMP);
    TempCollecter_Init(&tempCollecter[BACTROOM_OUT_TEMP], s_kTempCalculateParam1);
}

static double TempCollecterMap_GetResistanceValue1(TempCalibrateParam *tempCalibrateParam, Uint16 ad)
{
    double realV = ad * TEMPADCOLLECT_V_REF / TEMPADCOLLECT_AD_MAX; //根据AD值计算得到电压值
    double rt;
    if (realV >= 2.5)//超出PT1000计算范围
    {
        return 2000;
    }
    rt = ((tempCalibrateParam->vref - tempCalibrateParam->negativeInput)
            / (tempCalibrateParam->negativeInput - realV / 3)) * 1000;

    TRACE_CODE(" way 1 ad: %d ;", ad);
    TRACE_CODE("realV: ");
    System_PrintfFloat(TRACE_LEVEL_CODE, realV, 8);
    TRACE_CODE(" ; Rt: ");
    System_PrintfFloat(TRACE_LEVEL_CODE, rt, 8);
    TRACE_CODE(" ; Temp:");

    return rt;
}

static double TempCollecterMap_GetResistanceValue2(TempCalibrateParam *tempCalibrateParam, Uint16 ad)
{
    double realV = ad * TEMPADCOLLECT_V_REF / TEMPADCOLLECT_AD_MAX; //根据AD值计算得到电压值
    double rt;

    rt = (realV + 3.5 * tempCalibrateParam->negativeInput) * 150
            / (3.5 * (tempCalibrateParam->vref
                            - tempCalibrateParam->negativeInput) - realV);


    TRACE_CODE(" way 2 ad: %d ;", ad);
    TRACE_CODE("realV: ");
    System_PrintfFloat(TRACE_LEVEL_CODE, realV, 8);
    TRACE_CODE(" ; Rt: ");
    System_PrintfFloat(TRACE_LEVEL_CODE, rt, 8);
    TRACE_CODE(" ; Temp:");

    return rt;
}

char* TempCollecterMap_GetName(Uint8 index)
{
    static char name[20] = "";
    memset(name, 0, sizeof(name));
    switch(index)
    {
    case MEAROOM_TEMP:
        strcpy(name, "MeasureRoom");
        break;
    case BACTROOM_TEMP:
        strcpy(name, "BacteriaRoom");
        break;
    case MEAROOM_OUT_TEMP:
        strcpy(name, "MeasureRoomOut");
        break;
    case BACTROOM_OUT_TEMP:
        strcpy(name, "BacteriaRoomOut");
        break;
    default:
        strcpy(name, "NULL");
        break;
    }
    return name;
}
