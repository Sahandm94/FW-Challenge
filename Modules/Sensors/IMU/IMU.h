/*
 * IMU.h
 *
 *  Created on: 14Mar.,2020
 *      Author: Sahand Maleki
 */

#ifndef SENSORS_IMU_IMU_H_
#define SENSORS_IMU_IMU_H_

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "LSM6DSL/lsm6dsl_reg.h"
#include "LSM303AGR/lsm303agr_reg.h"
#include "stm32f4xx_hal.h"
#include "projectOptions.h"
#include "Utility/utility.h"
#include "Terminal/terminal.h"
#include "UI/ui.h"
#include "Flash/flash.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define IMU_STACK_SIZE 1024
#define IMU_MODULE_NAME "IMU"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 		 __  _  _  __  _    _  ___    ___ _   _  __  ____  ___
// 		|__] |  | |__] |    | |        |   \_/  |__] |___ [__
// 		|    |__| |__] |___ | |___     |    |   |    |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
typedef struct{
	uint32_t 	id;
	int16_t 	acceleration_mg[3];
	int16_t 	angular_rate_mdps[3];
	int16_t 	magnetic_mG[3];
	int16_t 	temperature_degC;
}tIMU_data;

typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

typedef union{
  int16_t i16bit;
  uint8_t u8bit[2];
} axis1bit16_t;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 	 __  _  _  __  _    _  ___    _  _ ____ ___ _  _  __   __   ___
// 	|__] |  | |__] |    | |       |\/| |___  |  |__| |  | |  \ [__
// 	|    |__| |__] |___ | |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
bool IMU_Init(void);
void I2C1_Init(void);
void I2C2_Init(void);
void IMU_Gyr_Init(void);
void IMU_Mag_Init(void);
void IMU_Acc_Init(void);
void IMU_parse_cmd(uint8_t enumCase, char *subcommand);

I2C_HandleTypeDef 	hi2c1;
osThreadId 			IMUTaskHandle;
QueueHandle_t 		IMUQueueHandle;
QueueHandle_t 		IMUCmdQueueHandle;



#endif /* SENSORS_IMU_IMU_H_ */
