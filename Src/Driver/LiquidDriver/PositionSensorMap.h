/*
 * PositionSensorMap.h
 *
 *  Created on: 2018年3月6日
 *      Author: LIANG
 */

#ifndef SRC_DRIVER_LIQUIDDRIVER_POSITIONSENSORMAP_H_
#define SRC_DRIVER_LIQUIDDRIVER_POSITIONSENSORMAP_H_

#include "PeristalticPump/SyringeManager.h"
#include "PeristalticPump/DisplacementMotorManager.h"

void PositionSensorMap_SyringeInit(Syringe *syringe);
void PositionSensorMap_DisplacementMotorInit(DisplacementMotor *displacementMotor);
void PositionSensorMap_CollisionSensorInit(PositionSensor *positionSensor);
void PositionSensorMap_WaterCheckSensorInit(PositionSensor *positionSensor);

#endif /* SRC_DRIVER_LIQUIDDRIVER_POSITIONSENSORMAP_H_ */
