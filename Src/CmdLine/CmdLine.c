/**
 * @addtogroup module_CmdLine
 * @{
 */

/**
 * @file
 * @brief 应用接口：命令行实现。
 * @details 定义各应用命令及其处理函数。
 * @version 1.0.0
 * @author kim.xiejinqiang
 * @date 2012-5-21
 */


#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdlib.h>
#include "Tracer/Trace.h"
#include "Console/Console.h"
#include "Driver/System.h"
#include "CmdLine.h"
#include "SystemConfig.h"
#include "DeviceIndicatorLED.h"
#include "UpdateDriver.h"
#include "DeviceUpdate/UpdateHandle.h"
#include "McuFlash.h"
#include "PeristalticPump/PeristalticPumpManager.h"
#include "PeristalticPump/SyringeManager.h"
#include "PeristalticPump/DCMotorManager.h"
#include "PeristalticPump/DisplacementMotorManager.h"
#include "Driver/LiquidDriver/PositionSensor.h"
#include "SolenoidValve/ValveManager.h"
#include "TemperatureControl/ThermostatDeviceManager.h"
#include "Manufacture/VersionInfo.h"
#include "OpticalControl/PMTControl.h"
#include "OpticalControl/StaticADControl.h"
#include "DNCP/App/DscpSysDefine.h"
#include "PeristalticPump/TMCConfig.h"
#include "Driver/LiquidDriver/TMCConfigDriver.h"
#include "PeristalticPump/StepperMotor.h"

// 命令行版本定义，命令有变更时，需要相应更新本版本号
const CmdLineVersion g_kCmdLineVersion =
{
        1,      // 主版本号
        0,      // 次版本号
        0,      // 修订版本号
        0       // 编译版本号
};
static  Uint8 s_currPumpNum = 0;

// 命令处理函数声明列表
static int Cmd_help(int argc, char *argv[]);
static int Cmd_welcome(int argc, char *argv[]);
static int Cmd_version(int argc, char *argv[]);
static int Cmd_showparam(int argc, char *argv[]);
static int Cmd_reset(int argc, char *argv[]);
static int Cmd_trace(int argc, char *argv[]);
static int Cmd_demo(int argc, char *argv[]);

//系统命令
static int Cmd_flash(int argc, char *argv[]);
static int Cmd_RestoreInit(int argc, char *argv[]);
static int Cmd_IAP(int argc, char *argv[]);
static int Cmd_SetBlink(int argc, char *argv[]);
static int Cmd_TaskState(int argc, char *argv[]);

//泵命令函数
static int Cmd_PumpNum(int argc, char *argv[]);
static int Cmd_PumpStatus(int argc, char *argv[]);
static int Cmd_PumpParam(int argc, char *argv[]);
static int Cmd_PumpFactor(int argc, char *argv[]);
static int Cmd_TotalPumps(int argc, char *argv[]);
static int Cmd_PumpVolume(int argc, char *argv[]);
static int Cmd_Pump(int argc, char *argv[]);
//阀命令函数
static int Cmd_valve(int argc, char *argv[]);

static int Cmd_Syringe(int argc, char *argv[]);
static int Cmd_DisplacementMotor(int argc, char *argv[]);
static int Cmd_DCMotor(int argc, char *argv[]);

static int Cmd_Motor(int argc, char *argv[]);
static int Cmd_TMC(int argc, char *argv[]);

//温控命令
static int Cmd_Therm(int argc, char *argv[]);
static int Cmd_Temp(int argc, char *argv[]);
static int Cmd_ThermParam(int argc, char *argv[]);
static int Cmd_TempFactor(int argc, char *argv[]);
static int Cmd_SetTempReport(int argc, char *argv[]);
static int Cmd_ThermDevice(int argc, char *argv[]);

//信号采集
static int Cmd_StaticAD(int argc, char *argv[]);
static int Cmd_PMT(int argc, char *argv[]);

static void InfoOutput(void *argument);
/**
 * @brief 命令行命令表，保存命令关键字与其处理函数之间的对应关系。
 * @details 每条命令对应一个结点，结点包括：命令关键字、处理函数、简短描述文本。
 */
const CmdLineEntry g_kConsoleCmdTable[] =
{
    { "demo",       Cmd_demo,       "\t\t: Demo for cmd implementation and param parse" },
    { "trace",      Cmd_trace,      "\t\t: Trace level" },
    { "welcome",    Cmd_welcome,    "\t\t: Display welcome message" },
    { "version",    Cmd_version,    "\t\t: Display version infomation about this application" },
    { "reset",      Cmd_reset,      "\t\t: Reset system" },
    { "help",       Cmd_help,       "\t\t: Display list of commands. Short format: h or ?" },

    { "flash",      Cmd_flash,      "\t\t: Flash read write erase operation." },
    { "RI",         Cmd_RestoreInit, "\t\t:Restore system initial state." },
    { "iap",        Cmd_IAP,         "\t\t:Provides erase and jump operations." },
    { "blink",      Cmd_SetBlink,    "\t\t:Set the duration of equipment indicator, on time and off time.Uint milliseconds." },
    { "taskstate",  Cmd_TaskState,  "\t\t: Out put system task state." },

    { "valve",      Cmd_valve,      "\t\t: valve set read operation." },
    { "pumpfactor", Cmd_PumpFactor, "\t\t: Setting and acquisition of pump calibration coefficient." },
    { "pumpstatus", Cmd_PumpStatus, "\t\t: Get the running state of the pump." },
    { "pumpparam",  Cmd_PumpParam,  "\t\t: Get the running state of the pump." },
    { "pumptotal",  Cmd_TotalPumps, "\t\t: Get pump total." },
    { "pumpvolume", Cmd_PumpVolume, "\t\t: Gets the volume of a pump on the current pump ." },
    { "pump",       Cmd_Pump,       "\t\t: The start and stop of the pump. " },
    { "pumpnum",    Cmd_PumpNum,    "\t\t: Sets the current pump number. " },

    { "syr",        Cmd_Syringe,   "\t\t: Syringe" },
    { "dpm",        Cmd_DisplacementMotor, "\t\t: x displacement motor and z displacement motor." },
    { "dcm",        Cmd_DCMotor,   "\t\t: DCMotor" },
    { "motor",        Cmd_Motor,   "\t\t: StepperMotor Absolutely Control" },
    { "tmc",       Cmd_TMC,       "\t\t: tmc register read or write. " },

    { "therm",      Cmd_Therm,      "\t\t: Thermostat temperature" },
    { "thermpid",   Cmd_ThermParam,"\t\t: The operation parameters of PID thermostat." },
    { "tempfactor", Cmd_TempFactor, "\t\t: Temperature sensor coefficient operation." },
    { "temp",       Cmd_Temp,       "\t\t: get temperature" },
    { "tempreport", Cmd_SetTempReport, "\t\t: Set the temperature reporting cycle." },
    { "thermdevice", Cmd_ThermDevice, "\t\t: " },

    { "staticad",        Cmd_StaticAD,        "\t\t: Static AD control operation ." },
    { "pmt",        Cmd_PMT,        "\t\t: PMT Signal acquisition operation ." },

    { "?",          Cmd_help,       0 },
    { "h",          Cmd_help,       0 },
    { "showparam",  Cmd_showparam,  0 },
    { 0, 0, 0 }
};


/**
 * @brief 判断第一个字串等于第二个字串。
 * @details 本函数与strcmp相比，预先做了有效性检查。
 * @param str1 要比较的字串1。
 * @param str2 要比较的字串2，不能为NULL。
 * @return 是否相等。
 */
Bool IsEqual(const char* str1, const char* str2)
{
    return (0 == strcmp(str1, str2)) ? TRUE : FALSE;
}

static xTaskHandle s_InfoOutputHandle;
static Uint16 s_getInfoTime = 0;
typedef enum
{
    LC,
    OA,
    TC,
    SYS,
}InfoOutputMode;

static InfoOutputMode s_InfoOutputMode = LC;

void CmdLine_Init(void)
{
    xTaskCreate(InfoOutput, "InfoOutput", CMDCLINE_INFOOUTPUT_STK_SIZE, NULL,
            CMDCLINE_INFOOUTPUT_TASK_PRIO, &s_InfoOutputHandle);
}

static void InfoOutput(void *argument)
{
    (void) argument;
    vTaskSuspend(NULL);
    while (1)
    {
        vTaskDelay(s_getInfoTime / portTICK_RATE_MS);
        switch(s_InfoOutputMode)
        {
            case LC:
                break;
            case OA:
                break;
            case SYS:
                System_TaskStatePrintf();
//                System_StateTimerValue();
                break;
        }
    }
}

//*****************************************************************************
//
// 系统常规命令处理函数
//
//*****************************************************************************
#include "console/port/driver/ConsoleUart.h"

int Cmd_TaskState(int argc, char *argv[])
{
    if(IsEqual(argv[1], "start"))
    {
        if (argv[2] && atoi(argv[2]) >= 10)
        {
            s_getInfoTime = atoi(argv[2]);
            s_InfoOutputMode = SYS;
            vTaskResume(s_InfoOutputHandle);
        }
        else
        {
            Printf("Invalid param %s\n", argv[2]);
        }
    }
    else if(IsEqual(argv[1], "stop"))
    {
        vTaskSuspend(s_InfoOutputHandle);
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== taskstate commands ======\n");
        Printf(" taskstate start [TIME]:ms\n");
        Printf(" taskstate stop        :\n");
    }
    return (0);
}


int Cmd_SetBlink(int argc, char *argv[])
{
    if (argv[1])
    {
        if (argv[2] && argv[3])
        {
            DeviceIndicatorLED_SetBlink(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== blink commands ======\n");
        Printf(" blink [DURATION] [ONTIME] [OFFTIME]:\n");
    }

    return (0);
}

int Cmd_IAP(int argc, char *argv[])
{
    if (IsEqual(argv[1], "erase"))
    {
        UpdateHandle_StartErase();
    }
    else if (IsEqual(argv[1], "write"))
    {
        if (argv[2] && argv[3] && argv[4])
        {
            UpdateHandle_WriteProgram((u8 *)argv[2], atoi(argv[3]), atoi(argv[4]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "check"))
    {
        if (argv[2])
        {
            UpdateHandle_CheckIntegrity(atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "read"))
    {
        if (argv[2] && argv[3])
        {
            uint8_t str[30]={""};
            UpdateDriver_Read(atoi(argv[2]),atoi(argv[3]),str);
            Printf("\n");
            for(int i = 0; i< atoi(argv[3]); i++)
            {
                Printf("0x%02x ",str[i]);
            }
            Printf("\n%s",str);
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "getmax"))
    {
        Printf("%d\n", UpdateHandle_GetMaxFragmentSize());
    }
    else if (IsEqual(argv[1], "getmode"))
    {
        DeviceRunMode mode = UpdateHandle_GetRunMode();
        Printf("%d\n", mode);
    }
#ifdef _CS_APP
    else if (IsEqual(argv[1], "updater"))
    {
        UpdateHandle_EnterUpdater();
    }
#else
    else if (IsEqual(argv[1], "app"))
    {
        UpdateHandle_EnterApplication();
    }
#endif
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== iap commands ======\n");
        Printf(" iap erase: \n");
        Printf(" iap write [TEXT]   [NUM]  [SEQ]: \n");
        Printf(" iap check [CRC16]              : \n");
        Printf(" iap read  [OFFSET] [NUM]       : \n");
#ifdef _CS_APP
        Printf(" iap updater                    : \n");
#else
        Printf(" iap app                        : \n");
#endif
        Printf(" iap getmax                     : \n");
        Printf(" iap getmode                    : \n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}
   
int Cmd_flash(int argc, char *argv[])
{
    if (IsEqual(argv[1], "deletewrite"))//不保留原始数据的写
    {
        if (argv[2] && argv[3] && argv[4])
        {
            McuFlash_DeleteWrite(atoi(argv[2]), atoi(argv[3]),(u8 *)argv[4]);
            Printf("\nWriteAddr 0x%x ",atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "write"))//保留原始数据的写
    {
        if (argv[2] && argv[3] && argv[4])
        {
            McuFlash_Write(atoi(argv[2]), atoi(argv[3]),(u8 *)argv[4]);
            Printf("\nWriteAddr 0x%x ",atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "read"))//读FLASH数据
    {
        if (argv[2] && argv[3])
        {
            uint8_t str[30]={""};
            McuFlash_Read(atoi(argv[2]),atoi(argv[3]),str);
            Printf("\nReadAddr 0x%x\n",atoi(argv[2]));
            for(int i = 0; i< atoi(argv[3]); i++)
            {
                Printf("0x%02x ",str[i]);
            }
            Printf("\n%s",str);
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "erase"))//擦除
    {
        if (argv[2])
        {
           McuFlash_EraseSector(atoi(argv[2]));
           Printf("\nEraseAddr 0x%x", atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== flash commands ======\n");
        Printf(" flash deletewrite [ADDR] [NUM] [TEXT]: \n");
        Printf(" flash write       [ADDR] [NUM] [TEXT]: \n");
        Printf(" flash read        [ADDR] [NUM]       : \n");
        Printf(" flash erase       [ADDR]             : \n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_RestoreInit(int argc, char *argv[])
{
    //以下操作可能会因为没有启动而控制台出现错误提醒。
    SyringeManager_Restore();
    PeristalticPumpManager_Restore();//依次关闭所有的泵
    DisplacementMotor_Restore();
    DCMotorManager_Restore();
    ValveManager_SetValvesMap(0);
//    ThermostatManager_RestoreInit();
//    ThermostatDeviceManager_RestoreInit();
    return 0;
}


//*****************************************************************************
//
// 阀命令处理函数
//
//*****************************************************************************
int Cmd_valve(int argc, char *argv[])
{
    if (IsEqual(argv[1], "map"))
    {
        if (argv[2] )
        {
            ValveManager_SetValvesMap(atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param");
        }
    }
    else if (IsEqual(argv[1], "open"))
    {
        if (argv[2] && atoi(argv[2]) > 0)
        {
            ValveManager_SetValvesMap((Uint32)( 1 << (atoi(argv[2]) - 1)));
        }
        else
        {
            Printf("Invalid param");
        }
    }
    else if (IsEqual(argv[1], "closeall"))
    {
        ValveManager_SetValvesMap(0);
    }
    else if (IsEqual(argv[1], "get"))
    {
        Uint32 map = ValveManager_GetValvesMap();
        Printf("ValvesMap: 0x%4x\n", map);
    }
    else if (IsEqual(argv[1], "total"))
    {
        Printf("total: %d\n", ValveManager_GetTotalValves());
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== valve commands ======\n");
        Printf(" valve map  [MAP]: Set the map of the valve. map must be 0x0 - 0x%x.\n\n", SOLENOIDVALVE_MAX_MAP);
        Printf(" valve open [NUM]: Open valve NUM.Num must be 1 - %d.\n", ValveManager_GetTotalValves());
        Printf(" valve closeall  : Close all valves. \n");
        Printf(" valve get       : Mapping map of the solenoid valve. \n");
        Printf(" valve total     : Total number of solenoid valves. \n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

//*****************************************************************************
//
// 泵命令处理函数
//
//*****************************************************************************
int Cmd_PumpNum(int argc, char *argv[])
{
    if (argv[1])
    {
        if (atoi(argv[1]) < PeristalticPumpManager_GetTotalPumps())
        {
            s_currPumpNum = atoi(argv[1]);
        }
        else
        {
            Printf("NUM must to be 0 - %d\n", PeristalticPumpManager_GetTotalPumps() - 1);
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") || IsEqual(argv[1], "?"))
    {
        Printf("====== pumpnum commands ======\n");
        Printf(
                " pumpnum [NUM]: Set the pump for the current operation to NUM.NUM must to be 0 - %d\n",
                PeristalticPumpManager_GetTotalPumps() - 1);
    }
    return (0);
}

int Cmd_PumpStatus(int argc, char *argv[])
{
    StepperMotorStatus status = PeristalticPumpManager_GetStatus(s_currPumpNum);
    switch(status)
    {
    case StepperMotor_IDLE:
        Printf("Pump: %d IDLE\n",s_currPumpNum);
        break;
    case StepperMotor_BUSY:
        Printf("Pump: %d BUSY\n",s_currPumpNum);
        break;
    }
    return (0);
}

int Cmd_PumpFactor(int argc, char *argv[])
{
    if (IsEqual(argv[1], "set"))
    {
        if (argv[2])
        {
            PeristalticPumpManager_SetFactor(s_currPumpNum, atof(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "get"))
    {
        float factor = PeristalticPumpManager_GetFactor(s_currPumpNum);
        Printf("Peristaltic Pump %d Factor ", s_currPumpNum);
        System_PrintfFloat(1, factor, 8);
        Printf(" ml/step");
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== pumpfactor commands ======\n");
        Printf(" pumpfactor set [FACTOR]: Set calibration FACTOR for current pump.Unit is ml/step\n");
        Printf(" pumpfactor get         : Calibration parameters for reading the current pump.\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_PumpParam(int argc, char *argv[])
{
    if (IsEqual(argv[1], "set"))
    {
        if (argv[2] && argv[3])
        {
            StepperMotorParam param;
            param.acceleration = atof(argv[2]);
            param.maxSpeed = atof(argv[3]);
            PeristalticPumpManager_SetDefaultMotionParam(s_currPumpNum, param);
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "get"))
    {
        StepperMotorParam param= PeristalticPumpManager_GetDefaultMotionParam(s_currPumpNum);

        Printf("Peristaltic Pump %d acc:", s_currPumpNum);
        System_PrintfFloat(1, param.acceleration, 4);
        Printf(" ml/(s^2),maxSpeed:");
        System_PrintfFloat(1, param.maxSpeed, 4);
        Printf(" ml/s\n");

        Printf("Default acc:");
        System_PrintfFloat(1, param.acceleration / PeristalticPumpManager_GetFactor(s_currPumpNum), 4);
        Printf(" step/(s^2),maxSpeed:");
        System_PrintfFloat(1, param.maxSpeed / PeristalticPumpManager_GetFactor(s_currPumpNum), 4);
        Printf(" step/s\n");
    }
    else if (IsEqual(argv[1], "setmoveing"))
    {
        if (argv[2] && argv[3])
        {
            StepperMotorParam param;
            param.acceleration = atof(argv[2]);
            param.maxSpeed = atof(argv[3]);
            PeristalticPumpManager_SetCurrentMotionParam(s_currPumpNum, param);
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("========== pumpparam commands ==========\n");
        Printf(" pumpparam set         [ACC] [MAXSPEED]: Set the ACC(ml/(s^2)) and MAXSPEED(ml/s) of the current pump, the motion parameters are saved to flash. \n");
        Printf(" pumpparam get                         : Gets the motion parameters of the current pump.\n");
        Printf(" pumpparam setmoveing  [ACC] [MAXSPEED]: Set temporary acceleration and speed of the current pump.\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_TotalPumps(int argc, char *argv[])
{
    Printf("TotalPumps: %d\n", PeristalticPumpManager_GetTotalPumps());
    return (0);
}

int Cmd_PumpVolume(int argc, char *argv[])
{
   if (IsEqual(argv[1], "change"))
   {
       if (argv[2])
       {
           PeristalticPumpManager_ChangeVolume(s_currPumpNum, atof(argv[2]));
       }
       else
       {
           Printf("Invalid param\n");
       }
   }
   else if (IsEqual(argv[1], "get"))
   {
       float volume = PeristalticPumpManager_GetVolume(s_currPumpNum);
       Printf("Pump %d Volume:", s_currPumpNum);
       System_PrintfFloat(1, volume, 4);
       Printf(" ml");
   }
   else if (0 == argv[1] || IsEqual(argv[1], "help") ||
            IsEqual(argv[1], "?"))
   {
       Printf("====== pumpvolume commands ======\n");
       Printf(" pumpvolume change [VOLUME]: Change the volume of the pump. Uint ml\n");
       Printf(" pumpvolume get            : drain VOLUME ml\n");
   }
   return (0);
}

int Cmd_Pump(int argc, char *argv[])
{

    Bool isParamOK = FALSE;
    Direction dir;

    if (IsEqual(argv[1], "extract") || IsEqual(argv[1], "e"))
    {
        isParamOK = TRUE;
        dir = FORWARD;
    }
    else if (IsEqual(argv[1], "drain") || IsEqual(argv[1], "d"))
    {
        isParamOK = TRUE;
        dir = BACKWARD;
    }
    if(TRUE == isParamOK)
    {
        if(argv[2])
        {
            if(argv[3] && argv[4])
            {
                StepperMotorParam param;
                param.acceleration = atof(argv[3]) * PeristalticPumpManager_GetFactor(s_currPumpNum);
                param.maxSpeed = atof(argv[4]) * PeristalticPumpManager_GetFactor(s_currPumpNum);
                if (DSCP_OK == PeristalticPumpManager_SetCurrentMotionParam(s_currPumpNum, param))
                {
                    PeristalticPumpManager_Start(s_currPumpNum, dir, atof(argv[2]), FALSE);
                }
            }
            else
            {
                PeristalticPumpManager_Start(s_currPumpNum, dir, atof(argv[2]), TRUE);
            }
        }
        else
        {
            Printf("Invalid param\n");
        }

    }
    else if (IsEqual(argv[1], "stop"))
    {
        PeristalticPumpManager_Stop(s_currPumpNum);
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== pump commands ======\n");
        Printf(" pump extract(e) [VOLUME] [ACC] [SPEED]: Extract VOLUME ml.ACC and SPEED is optional.Uint step/s.\n");
        Printf(" pump drain(d)   [VOLUME] [ACC] [SPEED]: drain VOLUME ml.ACC and SPEED is optional.Uint step/s. \n");
        Printf(" pump stop                             : \n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);

}

int Cmd_Syringe(int argc, char *argv[])
{
    Bool isParamOK = FALSE;
    Direction dir;

    if (IsEqual(argv[1], "extract") || IsEqual(argv[1], "e"))
    {
        isParamOK = TRUE;
        dir = FORWARD;
    }
    else if(IsEqual(argv[1], "drain") || IsEqual(argv[1], "d"))
    {
        isParamOK = TRUE;
        dir = BACKWARD;
    }
    if(TRUE == isParamOK)
    {
        if(argv[2])
        {
            if(argv[3] && argv[4])
            {
                StepperMotorParam param;
                param.acceleration = atof(argv[3]) * SyringeManager_GetFactor();
                param.maxSpeed = atof(argv[4]) * SyringeManager_GetFactor();
                if (DSCP_OK == SyringeManager_SetCurrentMotionParam(param))
                {
                    SyringeManager_Start(dir, atof(argv[2]), FALSE);
                }
            }
            else
            {
                SyringeManager_Start(dir, atof(argv[2]), TRUE);
            }
        }
        else
        {
            Printf("Invalid param\n");
        }

    }
    else if (IsEqual(argv[1], "reset"))
    {
        SyringeManager_Start(BACKWARD, 1, TRUE);
    }
    else if (IsEqual(argv[1], "stop"))
    {
        SyringeManager_RequestStop();
    }
    else if (IsEqual(argv[1], "sensor"))
    {
        if (TRUE == SyringeManager_IsSensorBlocked())
        {
            Printf ("Sensor Blocked\n");
        }
        else
        {
            Printf ("Sensor don't Blocked\n");
        }
    }
    else if (IsEqual(argv[1], "water"))
    {
        if(argv[2]&&(atoi(argv[2])==0))
        {
            if (TRUE == SyringeManager_IsWaterCheckSensorBlocked(0))
            {
                Printf ("WaterCheck Sensor 0  Blocked\n");
            }
            else
            {
                Printf ("WaterCheck Sensor 1  don't Blocked\n");
            }
        }
        else if(argv[2]&&(atoi(argv[2])==1))
        {
            if (TRUE == SyringeManager_IsWaterCheckSensorBlocked(1))
            {
                Printf ("WaterCheck Sensor 1 Blocked\n");
            }
            else
            {
                Printf ("WaterCheck Sensor 1  don't Blocked\n");
            }
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "param"))
    {
        if (IsEqual(argv[2], "set"))
         {
             if (argv[3] && argv[4])
             {
                 StepperMotorParam param;
                 param.acceleration = atof(argv[3]);
                 param.maxSpeed = atof(argv[4]);
                 SyringeManager_SetDefaultMotionParam(param);
             }
             else
             {
                 Printf("Invalid param\n");
             }
         }
         else if (IsEqual(argv[2], "get"))
         {
             StepperMotorParam param= SyringeManager_GetDefaultMotionParam();

             Printf("Syringe acc:");
             System_PrintfFloat(1, param.acceleration, 4);
             Printf(" ml/(s^2),maxSpeed:");
             System_PrintfFloat(1, param.maxSpeed, 4);
             Printf(" ml/s\n");

             Printf("Default acc:");
             System_PrintfFloat(1, param.acceleration / SyringeManager_GetFactor(), 4);
             Printf(" step/(s^2),maxSpeed:");
             System_PrintfFloat(1, param.maxSpeed / SyringeManager_GetFactor(), 4);
             Printf(" step/s\n");
         }
         else
         {
             Printf("Invalid param\n");
         }
    }
    else if (IsEqual(argv[1], "factor"))
    {
        if (IsEqual(argv[2], "set"))
        {
            if (argv[3])
            {
                SyringeManager_SetFactor(atof(argv[3]));
            }
            else
            {
                Printf("Invalid param\n");
            }
        }
        else if (IsEqual(argv[2], "get"))
        {
            float factor = SyringeManager_GetFactor();
            Printf("Syringe Factor ");
            System_PrintfFloat(1, factor, 8);
            Printf(" ml/step");
        }
    }
    else if (IsEqual(argv[1], "status"))
    {
        SyringeStatus status = SyringeManager_GetStatus();
        switch(status)
        {
        case SYRINGE_IDLE:
            Printf("Syringe IDLE\n");
            break;
        default:
            Printf("Syringe BUSY\n");
            break;
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== syr commands ======\n");
        Printf(" syr extract(e) [VOLUME] [ACC] [SPEED]: Extract VOLUME ml.ACC and SPEED is optional.Uint step/s.\n");
        Printf(" syr drain(d) [VOLUME] [ACC] [SPEED]: Drain VOLUME ml.ACC and SPEED is optional.Uint step/s.\n");
        Printf(" syr reset                            : \n");
        Printf(" syr stop                             : \n");
        Printf(" syr sensor                         : \n");
        Printf(" syr water [INDEX]           : water check sensor 0,1\n");
        System_Delay(10);
        Printf(" syr param set  [ACC] [MAXSPEED]      : Set the ACC(ml/(s^2)) and MAXSPEED(ml/s) of the Syringe, the motion parameters are saved to flash. \n");
        Printf(" syr param get                        : Gets the motion parameters of the Syringe.\n");
        Printf(" syr factor set [FACTOR]              : Set calibration FACTOR for Syringe.Unit is ml/step\n");
        System_Delay(10);
        Printf(" syr factor get                       : Calibration parameters for reading the Syringe.\n");
        Printf(" syr status                           : \n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_DisplacementMotor(int argc, char *argv[])
{
    Bool isParamOK = FALSE;
    Uint8 index = 0;

    if (IsEqual(argv[2], "collision") && argv[1])
    {
        index = atoi(argv[1]);
        if (DisplacementMotor_CheckCollision(index))
        {
            Printf("%d Collision is Blocked\n ", index);
        }
        else
        {
            Printf("%d Collision don't Blocked\n", index);
        }
        return (0);
    }
    if (IsEqual(argv[1], "x") || IsEqual(argv[1], "z") )
    {
        isParamOK = TRUE;
        if (IsEqual(argv[1], "x"))
        {
            index = X_DISPLACEMENTMOTOR;
        }
        else
        {
            index = Z_DISPLACEMENTMOTOR;
        }
    }
    if(TRUE == isParamOK)
    {
        if(IsEqual(argv[2], "start"))
        {
            if ((argv[3] && argv[4]))
            {
                isParamOK = FALSE;
                DisplacementMotorMode mode;
                if (IsEqual(argv[4], "absolute") || IsEqual(argv[4], "a"))
                {
                    mode = MOTOR_MOVE_ABSOLUTE_MODE;
                    isParamOK = TRUE;
                }
                else if (IsEqual(argv[4], "relative") || IsEqual(argv[4], "r"))
                {
                    mode = MOTOR_MOVE_RELATIVE_MODE;
                    isParamOK = TRUE;
                }
                else if (IsEqual(argv[4], "safe") || IsEqual(argv[4], "s"))
                {
                    mode = MOTOR_MOVE_SAFE_MODE;
                    isParamOK = TRUE;
                }
                if (TRUE == isParamOK)
                {
                    if(argv[5] && argv[6])
                    {
                        StepperMotorParam param;
                        param.acceleration = atof(argv[5]);
                        param.maxSpeed = atof(argv[6]);
                        if (DSCP_OK == DisplacementMotor_SetCurrentMotionParam(index, param))
                        {
                            DisplacementMotor_Start(index, atoi(argv[3]), mode, FALSE);
                        }
                    }
                    else
                    {
                        DisplacementMotor_Start(index, atoi(argv[3]), mode, TRUE);
                    }
                }
                else
                {
                    Printf("Invalid param: %s\n", argv[4]);
                }
            }
            else
            {
                Printf("Invalid param\n");
            }
        }
        else if (IsEqual(argv[2], "reset"))
        {
            DisplacementMotor_Reset(index);
        }
        else if (IsEqual(argv[2], "stop"))
        {
            DisplacementMotor_RequestStop(index);
        }
        else if (IsEqual(argv[2], "sensor"))
        {
            if (TRUE == DisplacementMotor_IsSensorBlocked(index))
            {
                Printf ("%s Sensor Blocked\n", DisplacementMotor_GetName(index));
            }
            else
            {
                Printf ("%s  Sensor don't Blocked\n", DisplacementMotor_GetName(index));
            }
        }
        else if (IsEqual(argv[2], "param"))
        {
            if (IsEqual(argv[3], "set"))
             {
                 if (argv[4] && argv[5])
                 {
                     StepperMotorParam param;
                     param.acceleration = atof(argv[4]);
                     param.maxSpeed = atof(argv[5]);
                     DisplacementMotor_SetDefaultMotionParam(index, param);
                 }
                 else
                 {
                     Printf("Invalid param\n");
                 }
             }
             else if (IsEqual(argv[3], "get"))
             {
                 StepperMotorParam param= DisplacementMotor_GetDefaultMotionParam(index);

                 Printf(" %s DisplacementMotor acc:", DisplacementMotor_GetName(index));
                 System_PrintfFloat(1, param.acceleration, 4);
                 Printf(" step/(s^2),maxSpeed:");
                 System_PrintfFloat(1, param.maxSpeed, 4);
                 Printf(" step/s\n");
             }
             else
             {
                 Printf("Invalid param\n");
             }
        }
        else if (IsEqual(argv[2], "status"))
        {
            DisplacementMotorStatus status = DisplacementMotor_GetStatus(index);
            switch(status)
            {
            case MOTOR_IDLE:
                Printf("%s IDLE\n", DisplacementMotor_GetName(index));
                break;
            default:
                Printf("%s BUSY\n", DisplacementMotor_GetName(index));
                break;
            }
        }
        else if (IsEqual(argv[2], "pos"))
        {
            Uint32 step = DisplacementMotor_GetCurrentSteps(index);
            Printf("%s Current Steps %d\n", DisplacementMotor_GetName(index), step);
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== dpm(DisplacementMotor) commands ======\n");
        Printf(" dpm [INDEX] start [STEP] [MODE] [ACC] [SPEED]: INDEX:x,z.MODE:absolute(a),relative(r),safe(s).ACC and SPEED is optional.Uint step/s.\n");
        Printf(" dpm [INDEX] reset                            : \n");
        System_Delay(10);
        Printf(" dpm [INDEX] stop                             : \n");
        Printf(" dpm [INDEX] sensor                           : \n");
        Printf(" dpm [NUM] collision                          : NUM:0,1\n");
        System_Delay(10);
        Printf(" dpm [INDEX] param set  [ACC] [MAXSPEED]      : Set the ACC(step/(s^2)) and MAXSPEED(step/s) of the DisplacementMotor, the motion parameters are saved to flash. \n");
        Printf(" dpm [INDEX] param get                        : Gets the motion parameters of the DisplacementMotor.\n");
        System_Delay(10);
        Printf(" dpm [INDEX] status                           : \n");
        Printf(" dpm [INDEX] pos                              : current Steps\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

static int Cmd_DCMotor(int argc, char *argv[])
{
      if (IsEqual(argv[1], "start"))
      {
          if(argv[2] && argv[3])
          {
              DCMotorManager_Start(atoi(argv[2]), atof(argv[3]));
          }
          else
          {
              Printf("Invalid param\n");
          }

      }
      else if (IsEqual(argv[1], "stop"))
      {
          if (argv[2])
          {
              DCMotorManager_RequestStop(atoi(argv[2]));
          }
          else
          {
              Printf("Invalid param\n");
          }
      }
      else if (IsEqual(argv[1], "volume"))
      {
          if (argv[2])
          {
              float volume = DCMotorManager_GetVolume(atoi(argv[2]));
              Printf("DCMotor %d volume: ", atoi(argv[2]));
              System_PrintfFloat(1, volume, 4);
              Printf(" ml\n ");
          }
          else
          {
              Printf("Invalid param\n");
          }
      }
      else if (IsEqual(argv[1], "status"))
      {
          if (argv[2])
          {
              if (DCMotorManager_IsRunning(atoi(argv[2])))
              {
                  Printf("DCMotor %d BUSY\n", atoi(argv[2]));
              }
              else
              {
                  Printf("DCMotor %d IDLE\n", atoi(argv[2]));
              }
          }
          else
          {
              Printf("Invalid param\n");
          }
      }
      else if (0 == argv[1] || IsEqual(argv[1], "help") ||
               IsEqual(argv[1], "?"))
      {
          Printf("====== dcm(DCMotor) commands ======\n");
          Printf(" dcm start [INDEX] [VOLUME]: INDEX:0 - %d.Volume uint is ml\n", DCMOTOR_TOTAL_PUMP - 1);
          Printf(" dcm stop  [INDEX]         : \n");
          Printf(" dcm volume [INDEX]        : \n");
          Printf(" dcm status [INDEX]        : \n");
      }
      else
      {
          Printf("Invalid param: %s\n", argv[1]);
      }
      return (0);
}

int Cmd_Motor(int argc, char *argv[])
{
    if (IsEqual(argv[1], "start"))
    {
        if(argv[2] && argv[3] && argv[4] && argv[5] && argv[6])
        {
            StepperMotor_AbsolutelyMove(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
        }
        else
        {
            Printf("Invalid param\n");
        }

    }
    else if (IsEqual(argv[1], "stop"))
    {
        if (argv[2])
        {
            StepperMotor_AbsoultelyStop(atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "num"))
    {
        Printf("Stepper Motor Total Number is %d \n", StepperMotor_GetTotalNumber);
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") || IsEqual(argv[1], "?"))
    {
        Printf("====== motor absolutely commands ======\n");
        Printf(" motor start [INDEX] [DIR] [STEP] [SPEED] [ACC]: INDEX:0 - %d. DIR:0=forward,1=backward\n", STEPPERMOTOR_TOTAL_NUMBER - 1);
        Printf(" motor stop [INDEX]         : \n");
        Printf(" motor num        : Get Motor Total Numbers\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }

}


int Cmd_TMC(int argc, char *argv[])
{
    Uint32 data;

    if (IsEqual(argv[1], "list"))
     {
         if (argv[2])
         {
             TMCConfig_RegList(atoi(argv[2]));
         }
         else
         {
             Printf("Invalid param");
         }
     }
    else if (IsEqual(argv[1], "init"))
    {
        TMCConfig_Reinit();
    }
    else if (IsEqual(argv[1], "read"))
     {
         if (argv[2] && argv[3])
         {
             if(TRUE == TMCConfig_ReadData(atoi(argv[2]), atoi(argv[3]), &data))
             {
                 Printf("\nTMC Read Data  = 0x%08X", data);
             }
         }
         else
         {
             Printf("Invalid param");
         }
     }
     else if (IsEqual(argv[1], "write"))
     {
         if (argv[2] && argv[3] && argv[4])
         {
             TMCConfig_WriteData(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
         }
         else
         {
             Printf("Invalid param");
         }
     }
     else if (IsEqual(argv[1], "err"))
     {
         if (argv[2])
         {
             TMCConfig_ReadDriveError(atoi(argv[2]));
         }
         else
         {
             Printf("Invalid param");
         }
     }
     else if (IsEqual(argv[1], "div"))
     {
         if (IsEqual(argv[2], "get") && argv[3])
         {
             TMCConfig_ReadSubdivision(atoi(argv[3]));
         }
         else if(IsEqual(argv[2], "set") && argv[3] && argv[4])
         {
             TMCConfig_WriteSubdivision(atoi(argv[3]), atoi(argv[4]));
         }
         else
         {
             Printf("Invalid param");
         }
     }
     else if (IsEqual(argv[1], "cur"))
     {
         if(IsEqual(argv[2], "set") && argv[3] && argv[4] && argv[5] && argv[6])
         {
             TMCConfig_CurrentSet(atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
         }
         else
         {
             Printf("Invalid param");
         }
     }
     else if (0 == argv[1] || IsEqual(argv[1], "help") ||
              IsEqual(argv[1], "?"))
     {
         Printf("====== SpinValve commands ======\n");
         Printf(" tmc init       : all tmc driver reinit . \n");
         Printf(" tmc list [SLAVE]        : read tmc driver [n] all register . \n");
         Printf(" tmc err [SLAVE]        : read tmc driver [n] error . \n");
         System_Delay(5);
         Printf(" tmc div get[SLAVE]      : tmc read subdivision. \n");
         Printf(" tmc div set [SLAVE] [DIV]    : tmc write subdivision. \n");
         Printf(" tmc cur set [SLAVE][IHOLD][IRUN][DLY]  : tmc driver current set. \n");
         System_Delay(5);
         Printf(" tmc read [SLAVE] [REG]    : tmc read register. \n");
         Printf(" tmc write [SLAVE] [REG] [DATA]     : tmc write register. \n");
     }
     else
     {
         Printf("Invalid param: %s\n", argv[1]);
     }
     return (0);
}

//*****************************************************************************
//
// 温控命令处理函数
//
//*****************************************************************************
int Cmd_Therm(int argc, char *argv[])
{
    if (IsEqual(argv[1], "auto"))
    {
        if(argv[2] && argv[3] && argv[4] && argv[5])
        {
            ThermostatManager_SendEventClose(atoi(argv[2]));
            ThermostatManager_Start(atoi(argv[2]),
                    THERMOSTAT_MODE_AUTO, atof(argv[3]), atof(argv[4]), atof(argv[5]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "heater"))
    {
        if(argv[2] && argv[3] && argv[4] && argv[5])
        {
            ThermostatManager_SendEventClose(atoi(argv[2]));
            ThermostatManager_Start(atoi(argv[2]),
                    THERMOSTAT_MODE_HEATER, atof(argv[3]), atof(argv[4]), atof(argv[5]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "refrigerate"))
    {
        if(argv[2] && argv[3] && argv[4] && argv[5])
        {
            ThermostatManager_SendEventClose(atoi(argv[2]));
            ThermostatManager_Start(atoi(argv[2]),
                    THERMOSTAT_MODE_REFRIGERATE, atof(argv[3]), atof(argv[4]), atof(argv[5]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "natural"))
    {
        if(argv[2] && argv[3] && argv[4] && argv[5])
        {
            ThermostatManager_SendEventClose(atoi(argv[2]));
            ThermostatManager_Start(atoi(argv[2]),
                    THERMOSTAT_MODE_NATURAL, atof(argv[3]), atof(argv[4]), atof(argv[5]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "stop"))
    {
        if(argv[2])
        {
            ThermostatManager_SendEventClose(atoi(argv[2]));
            ThermostatManager_RequestStop(atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "status"))
    {
        if(argv[2])
        {
            ThermostatStatus result = ThermostatManager_GetStatus(atoi(argv[2]));
            if (THERMOSTAT_IDLE == result)
            {
                Printf("%s Thermostat IDLE \n", ThermostatManager_GetName(atoi(argv[2])));
            }
            else
            {
                Printf("%s Thermostat BUSY \n", ThermostatManager_GetName(atoi(argv[2])));
            }
        }
        else
        {
            Printf("Invalid param\n");
        }
    }
    else if (IsEqual(argv[1], "number"))
    {
        Printf("\n GetTotalThermostat %d", TOTAL_THERMOSTAT);
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== therm commands ======\n");
        Printf(" therm auto        [INDEX] [TEMP] [ALW] [TIME]: \n");
        Printf(" therm heater      [INDEX] [TEMP] [ALW] [TIME]: \n");
        Printf(" therm refrigerate [INDEX] [TEMP] [ALW] [TIME]: \n");
        System_Delay(2);
        Printf(" therm natural     [INDEX] [TEMP] [ALW] [TIME]: \n");
        Printf(" therm stop        [INDEX]                    : \n");
        Printf(" therm status      [INDEX]                    : \n");
        Printf(" therm number                                 : \n");
        System_Delay(10);
        for (Uint8 i = 0; i < TOTAL_THERMOSTAT; i++)
        {
            Printf(" No %d name %s\n", i, ThermostatManager_GetName(i));
        }
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }

    return (0);
}

int Cmd_Temp(int argc, char *argv[])
{
    if (IsEqual(argv[1], "get"))
    {
        if(argv[2])
        {
            float temp = TempCollecterManager_GetTemp(atoi(argv[2]));
            Printf("\n %s : ",  TempCollecterManager_GetName(atoi(argv[2])));
            System_PrintfFloat(1, temp, 1);
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (IsEqual(argv[1], "env"))
    {
        Printf("\n EnvironmentTemp : ");
        System_PrintfFloat(1, TempCollecterManager_GetEnvironmentTemp(), 1);
    }
    else if (IsEqual(argv[1], "all"))
    {
        Printf("\n EnvironmentTemp : ");
        System_PrintfFloat(1, TempCollecterManager_GetEnvironmentTemp(), 1);

        for (Uint8 i = 0; i < TOTAL_TEMP; i++)
        {
            float temp = TempCollecterManager_GetTemp(i);
            Printf("\n %s Temp : ", TempCollecterManager_GetName(i));
            System_PrintfFloat(1, temp, 1);
        }
    }
    else if (IsEqual(argv[1], "factor"))
    {
        TempCalibrateParam calibrateFactor;
        if (IsEqual(argv[2], "get"))
        {
            if (argv[3])
            {
                calibrateFactor = TempCollecterManager_GetCalibrateFactor(atoi(argv[3]));
                TRACE_INFO("\n %s Temp \n negativeInput = ", TempCollecterManager_GetName(atoi(argv[3])));
                System_PrintfFloat(1, calibrateFactor.negativeInput, 8);
                Printf("\n vref =");
                System_PrintfFloat(1, calibrateFactor.vref, 8);
                Printf("\n vcal =");
                System_PrintfFloat(1, calibrateFactor.vcal, 8);
            }
            else
            {
                Printf("Invalid param!\n");
            }
        }
        else if (IsEqual(argv[2], "set"))
        {
            if (argv[3] && argv[4] && argv[5] && argv[6])
            {
                calibrateFactor.negativeInput = atof(argv[4]);
                calibrateFactor.vref = atof(argv[5]);
                calibrateFactor.vcal = atof(argv[6]);
                TempCollecterManager_SetCalibrateFactor(atoi(argv[3]), calibrateFactor);
                Printf("\n set ok");
            }
            else
            {
                Printf("Invalid param!\n");
            }
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== temp commands ======\n");
        Printf(" temp get [ID]                         :\n");
        Printf(" temp env                              :\n");
        Printf(" temp all                              :\n");
        Printf(" temp factor get [INDEX]               :\n");
        Printf(" temp factor set [INDEX] [NI] [VR] [VC]:\n");
        for (Uint8 i = 0; i < TOTAL_TEMP; i++)
        {
            Printf(" No %d name %s\n", i, TempCollecterManager_GetName(i));
        }
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_SetTempReport(int argc, char *argv[])
{
    if (argv[1] && atof(argv[1]) >= 0)
    {
        ThermostatManager_SetTempReportPeriod(atof(argv[1]));
        Printf("\n set ok");
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== tempreport commands ======\n");
        Printf(" tempreport [TIME] :\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_TempFactor(int argc, char *argv[])
{
    TempCalibrateParam calibrateFactor;
    if (IsEqual(argv[1], "get"))
    {
        if (argv[2])
        {
            calibrateFactor = ThermostatManager_GetCalibrateFactor(atoi(argv[2]));
            TRACE_INFO("\n %s Thermostat \n negativeInput = ", ThermostatManager_GetName(atoi(argv[2])));
            System_PrintfFloat(1, calibrateFactor.negativeInput, 8);
            Printf("\n vref =");
            System_PrintfFloat(1, calibrateFactor.vref, 8);
            Printf("\n vcal =");
            System_PrintfFloat(1, calibrateFactor.vcal, 8);
        }
        else
        {
            Printf("Invalid param!\n");
        }
    }
    else if (IsEqual(argv[1], "set"))
    {
        if (argv[2] && argv[3] && argv[4] && argv[5])
        {
            calibrateFactor.negativeInput = atof(argv[3]);
            calibrateFactor.vref = atof(argv[4]);
            calibrateFactor.vcal = atof(argv[5]);
            ThermostatManager_SetCalibrateFactor(atoi(argv[2]), calibrateFactor);
            Printf("\n set ok");
        }
        else
        {
            Printf("Invalid param!\n");
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== tempfactor commands ======\n");
        Printf(" tempfactor get [INDEX]               :\n");
        Printf(" tempfactor set [INDEX] [NI] [VR] [VC]:\n");
        System_Delay(10);
        for (Uint8 i = 0; i < TOTAL_THERMOSTAT; i++)
        {
            Printf(" No %d name %s\n", i, ThermostatManager_GetName(i));
        }
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

int Cmd_ThermParam(int argc, char *argv[])
{
   ThermostatParam thermostatparam;
   if (IsEqual(argv[1], "get"))
   {
       if (argv[2])
       {
           thermostatparam = ThermostatManager_GetPIDParam(atoi(argv[2]));
           Printf("\n %s Thermostat p =", ThermostatManager_GetName(atoi(argv[2])));
           System_PrintfFloat(1, thermostatparam.proportion, 3);
           Printf("\n i =");
           System_PrintfFloat(1, thermostatparam.integration, 3);
           Printf("\n d =");
           System_PrintfFloat(1, thermostatparam.differential, 3);
       }
       else
       {
           Printf("Invalid param: %s\n", argv[2]);
       }
   }
   else if (IsEqual(argv[1], "set"))
   {
       if (argv[2] && argv[3] && argv[4] && argv[5])
       {
           thermostatparam.proportion = atof(argv[3]);
           thermostatparam.integration = atof(argv[4]);
           thermostatparam.differential = atof(argv[5]);
           Printf("\n set %d ", ThermostatManager_SetPIDParam(atoi(argv[2]), thermostatparam));
       }
       else
       {
           Printf("Invalid param: 1:%s 2:%s 3:%s 4:%s\n", argv[2], argv[3], argv[4], argv[5]);
       }
   }
   else if (IsEqual(argv[1], "getcur"))
   {
       if (argv[2])
       {
           thermostatparam = ThermostatManager_GetCurrentPIDParam(atoi(argv[2]));
           Printf("\n %s Thermostat p =", ThermostatManager_GetName(atoi(argv[2])));
           System_PrintfFloat(1, thermostatparam.proportion, 3);
           Printf("\n i =");
           System_PrintfFloat(1, thermostatparam.integration, 3);
           Printf("\n d =");
           System_PrintfFloat(1, thermostatparam.differential, 3);
       }
       else
       {
           Printf("Invalid param: %s\n", argv[2]);
       }
   }
   else if (IsEqual(argv[1], "setcur"))
   {
       if (argv[2] && argv[3] && argv[4] && argv[5])
       {
           thermostatparam.proportion = atof(argv[3]);
           thermostatparam.integration = atof(argv[4]);
           thermostatparam.differential = atof(argv[5]);
           Printf("\n set %d ", ThermostatManager_SetCurrentPIDParam(atoi(argv[2]), thermostatparam));
       }
       else
       {
           Printf("Invalid param: 1:%s 2:%s 3:%s 4:%s\n", argv[2], argv[3], argv[4], argv[5]);
       }
   }
   else if (0 == argv[1] || IsEqual(argv[1], "help") || IsEqual(argv[1], "?"))
   {
       Printf("====== thermpid commands ======\n");
       Printf(" thermpid get    [INDEX]            :\n");
       Printf(" thermpid set    [INDEX] [P] [I] [D]:\n");
       Printf(" thermpid getcur [INDEX]            :\n");
       Printf(" thermpid setcur [INDEX] [P] [I] [D]:\n");
       System_Delay(10);
       for (Uint8 i = 0; i < TOTAL_THERMOSTAT; i++)
       {
           Printf(" No %d name %s\n", i, ThermostatManager_GetName(i));
       }
   }
   else
   {
       Printf("Invalid param: %s\n", argv[1]);
   }
   return (0);
}

int Cmd_ThermDevice(int argc, char *argv[])
{
    if (IsEqual(argv[1], "level"))
    {
        if(argv[2] && argv[3])
        {
            Printf("%s Device SetOutput :", ThermostatDeviceManager_GetName(atoi(argv[2])));
            System_PrintfFloat(1, atof(argv[3]) * 100, 3);
            Printf(" %%");
            ThermostatDeviceManager_SetOutput(atoi(argv[2]), atof(argv[3]));
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (IsEqual(argv[1], "stop"))
    {
        if(argv[2])
        {
            Printf("%s Device stop", ThermostatDeviceManager_GetName(atoi(argv[2])));
            ThermostatDeviceManager_SetOutput(atoi(argv[2]), 0);
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (IsEqual(argv[1], "isopen"))
    {
        if(argv[2])
        {
            Printf("\n %s ThermDevice ", ThermostatDeviceManager_GetName(atoi(argv[2])));
            ThermostatDeviceManager_IsOpen(atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (IsEqual(argv[1], "setduty"))
    {
        if(argv[2] && argv[3])
        {
            Printf("\n %s Device  SetHeaterMaxDutyCycle :", ThermostatDeviceManager_GetName(atoi(argv[2])));
            System_PrintfFloat(1, atof(argv[3]) * 100, 3);
            Printf(" %%");
            ThermostatDeviceManager_SetMaxDutyCycle(atoi(argv[2]), atof(argv[3]));
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (IsEqual(argv[1], "getduty"))
    {
        if(argv[2])
        {
            Printf("\n %s Device max duty cycle:", ThermostatDeviceManager_GetName(atoi(argv[2])));
            ThermostatDeviceManager_GetMaxDutyCycle(atoi(argv[2]));
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
            IsEqual(argv[1], "?"))
    {
        Printf("====== thermdevice commands ======\n");
        Printf(" thermdevice level   [INDEX] [PER] : 0 - %d device output\n", TOTAL_THERMOSTATDEVICE);
        Printf(" thermdevice stop    [INDEX]       :\n");
        Printf(" thermdevice isopen  [INDEX]       :\n");
        Printf(" thermdevice setduty [INDEX] [DUTY]:\n");
        Printf(" thermdevice getduty [INDEX]       :\n");
        for (Uint8 i = 0; i < TOTAL_THERMOSTATDEVICE; i++)
        {
            Printf(" No %d name %s\n", i, ThermostatDeviceManager_GetName(i));
            System_Delay(1);
        }
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }
    return (0);
}

//*****************************************************************************
//
// 测量信号命令处理函数
//
//*****************************************************************************
int Cmd_StaticAD(int argc, char *argv[])
{
    if (IsEqual(argv[1], "start"))
    {
        if(argv[2])
        {
            if(argv[3])
            {
                StaticADControl_Start(atoi(argv[2]), atoi(argv[3]));
            }
            else
            {
                Printf("Lack of param target\n");
            }
        }
        else
        {
            Printf("Lack of param index\n");
        }
    }
    else if (IsEqual(argv[1], "stop"))
    {
        StaticADControl_Stop();
    }
    else if (IsEqual(argv[1], "isValid"))
    {
        if(TRUE == StaticADControl_IsValid())
        {
            Printf("AD control is valid\n");
        }
        else
        {
            Printf("AD control is invalid\n");
        }
    }
    else if (IsEqual(argv[1], "get"))
    {
        if(IsEqual(argv[2], "default"))
        {
            if(argv[3])
            {
                Printf("Get ad5175 %d default value is %d\n", atoi(argv[3]), StaticADControl_GetDefaultValue(atoi(argv[3])));
            }
            else
            {
                Printf("Lack of param index\n");
            }
        }
        else if(IsEqual(argv[2], "real"))
        {
            if(argv[3])
            {
                Printf("Get ad5175 %d real value is %d\n", atoi(argv[3]), StaticADControl_GetRealValue(atoi(argv[3])));
            }
            else
            {
                Printf("Lack of param index\n");
            }
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (IsEqual(argv[1], "set"))
    {
        if(IsEqual(argv[2], "default"))
        {
            if(argv[3]&&argv[4])
            {
                StaticADControl_SetDefaultValue(atoi(argv[3]), atoi(argv[4]));
            }
            else
            {
                Printf("Invalid param index or value\n");
            }
        }
        else if(IsEqual(argv[2], "real"))
        {
            if(argv[3]&&argv[4])
            {
                StaticADControl_SetRealValue(atoi(argv[3]), atoi(argv[4]));
            }
            else
            {
                Printf("Invalid param index or value\n");
            }
        }
        else
        {
            Printf("Invalid param: %s\n", argv[2]);
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== Static AD Control Commands ======\n");
        Printf(" staticad start  [index] [target] : start static ad control. [index: pmt_ref=0,pmt_mea=1]\n");
        Printf(" staticad stop : staticad control stop \n");
        System_Delay(3);
        Printf(" staticad get default  [index] : get default value from flash \n");
        Printf(" staticad set default  [index] [value] : set default value to flash \n");
        System_Delay(3);
        Printf(" staticad get real  [index] : get rdac real value\n");
        Printf(" staticad set real  [index] [value] : set rdac real value \n");
        Printf(" staticad isValid : return ad control is valid or invalid \n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }

    return (0);
}

int Cmd_PMT(int argc, char *argv[])
{
    if (IsEqual(argv[1], "start"))
    {
            if (argv[2] && atoi(argv[2]) >= 1)
            {
                PMTControl_StartAcquire(atoi(argv[2]), FALSE);
            }
            else
            {
                Printf("Invalid param\n");
            }
     }
    else if(IsEqual(argv[1], "stop"))
    {
        PMTControl_StopAcquire(FALSE);
    }
    else if(IsEqual(argv[1], "status"))
    {
        Printf("PMT control status is %d\n", PMTControl_GetStatus());
    }
    else if(IsEqual(argv[1], "power"))
    {
        if (IsEqual(argv[2],"on"))
        {
            PMTControl_PowerOn();
        }
        else if(IsEqual(argv[2],"off"))
        {
            PMTControl_PowerOff();
        }
        else if(IsEqual(argv[2],"status"))
        {
            if(PMTControl_PowerStatus())
            {
                Printf("PMT  power is on\n");
            }
            else
            {
                Printf("PMT power is off\n");
            }
        }
        else
        {
            Printf("Invalid  Param\n");
        }
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||IsEqual(argv[1], "?"))
    {
        Printf("====== pmt commands ======\n");
        Printf(" pmt start  [TIME]        :start pmt signal acquire [TIME] ms.\n");
        Printf(" pmt stop                       : stop pmt acquire\n");
        Printf(" pmt status                    : get pmt control status 0-idle 1-busy.\n");
        Printf(" pmt power on             : open the pmt power\n");
        Printf(" pmt power off             : close  the pmt power\n");
        Printf(" pmt power status       : get the power status\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }

    return (0);
}

//*****************************************************************************
//
// 命令处理函数
//
//*****************************************************************************
// 显示帮助，简单显示命令列表
int Cmd_help(int argc, char *argv[])
{
    CmdLineEntry *cmdEntry;

    ConsoleOut("\nAvailable commands\n");
    ConsoleOut("------------------\n");

    cmdEntry = (CmdLineEntry *) &g_kConsoleCmdTable[0];

    // 遍历整个命令表，打印出有提示信息的命令
    while (cmdEntry->cmdKeyword)
    {
        // 延时一下，等待控制台缓冲区空出足够的空间
        System_Delay(10);

        if (cmdEntry->cmdHelp)
            ConsoleOut("%s%s\n", cmdEntry->cmdKeyword, cmdEntry->cmdHelp);

        cmdEntry++;
    }

    return (0);
}

int Cmd_version(int argc, char *argv[])
{
    ManufVersion ver = VersionInfo_GetSoftwareVersion();
    ConsoleOut("Version: %d.%d.%d.%d\n",
            ver.major,
            ver.minor,
            ver.revision,
            ver.build
            );
    return(0);
}

int Cmd_welcome(int argc, char *argv[])
{
    Console_Welcome();
    return(0);
}


// 显示参数
int Cmd_showparam(int argc, char *argv[])
{
    int i = 0;

    ConsoleOut("The params is:\n");
    for (i = 1; i < argc; i++)
    {
        ConsoleOut("    Param %d: %s\n", i, argv[i]);
    }

    return(0);
}


int Cmd_reset(int argc, char *argv[])
{
    Printf("\n\n\n");
    System_Delay(10);
    System_Reset();
    return (0);
}

int Cmd_trace(int argc, char *argv[])
{
    if (argv[1])
    {
        Trace_SetLevel(atoi(argv[1]));
    }
    else
    {
        Printf("L: %d\n", Trace_GetLevel());
    }

    return (0);
}

// 命令处理函数示例
int Cmd_demo(int argc, char *argv[])
{
    if (IsEqual(argv[1], "subcmd1"))
    {
        if (IsEqual(argv[2], "param"))
        {
            // 调用相关功能函数
            Printf("Exc: subcmd1 param");
        }
    }
    else if (IsEqual(argv[1], "subcmd2"))
    {
        Printf("Exc: subcmd2");
    }
    else if (0 == argv[1] || IsEqual(argv[1], "help") ||
             IsEqual(argv[1], "?"))
    {
        Printf("====== Sub commands ======\n");
        Printf(" mycmd subcmd1 param : Sub command description\n");
        Printf(" mycmd subcmd2       : Sub command description\n");
    }
    else
    {
        Printf("Invalid param: %s\n", argv[1]);
    }

    return (0);
}


/** @} */
