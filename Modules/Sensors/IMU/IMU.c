/*
 * IMU.c
 *
 *  Created on: 14Mar.,2020
 *      Author: Sahand Maleki
 */

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "IMU.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define SAMPLING_PERIOD_MS	500
#define IMU_INVALID_ID		0xFFFFFFFF

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  __   __  _ _  _ __  ___ ____    _  _ ____ ___ _  _  __   __   ___
// |__] |__/ | |  ||__|  |  |___    |\/| |___  |  |__| |  | |  \ [__
// |    |  \ |  \/ |  |  |  |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static void 		IMU_GPIO_Init(void);
static void 		IMU_Peripheral_Init(void);
static int32_t 		IMU_platform_write(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len);
static int32_t 		IMU_platform_read(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len);
static void 		IMUThread(void const * argument);
static void 		IMUsamplingTimerCallback(TimerHandle_t xTimer);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//				_  _ __   __  _ __   __  _    ____  ___
//				|  ||__| |__/ ||__| |__] |    |___ [__
//				 \/ |  | |  \ ||  | |__] |___ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
TimerHandle_t samplingTimer;

enum{
	DISPLAY_LAST_RECORD = 0,
	DISPLAY_RECORDS,
};
static tTerminal_ModuleCmdHandle imuTerminalCommands_Handle = {
							.commands = {"display_last_record\0display_records\0"},
							.name = IMU_MODULE_NAME,
							.moduleParseCmdFunction = IMU_parse_cmd
							};

static SemaphoreHandle_t I2C_Semaphore;

static axis3bit16_t 	data_raw_acceleration;
static axis3bit16_t 	data_raw_angular_rate;
static axis3bit16_t 	data_raw_magnetic;
static axis1bit16_t 	data_raw_temperature;
static tIMU_data 		IMU_data = {.id = 0};
static stmdev_ctx_t 	Magnetometer 	= { .write_reg 	= IMU_platform_write,
											.read_reg 	= IMU_platform_read,
											.handle		= (void*)LSM303AGR_I2C_ADD_MG};

static stmdev_ctx_t 	Accelerometer 	= { .write_reg 	= IMU_platform_write,
											.read_reg 	= IMU_platform_read,
											.handle		= (void*)LSM303AGR_I2C_ADD_XL};

static stmdev_ctx_t 	Gyroscope 		= {	.write_reg = IMU_platform_write,
											.read_reg = IMU_platform_read,
											.handle = (void *)LSM6DSL_I2C_ADD_H};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// _  _ ____ ___ _  _  __   __     _ _  _  __  _    ____ _  _ ____ _  _ ___ __  ___ _  __  _  _  ___
// |\/| |___  |  |__| |  | |  \    | |\/| |__] |    |___ |\/| |___ |\ |  | |__|  |  | |  | |\ | [__
// |  | |___  |  |  | |__| |__/    | |  | |    |___ |___ |  | |___ | \|  | |  |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool IMU_Init(void)
{
	IMU_GPIO_Init();
	IMU_Peripheral_Init();
	IMU_Gyr_Init();
	IMU_Acc_Init();
	IMU_Mag_Init();

	samplingTimer = xTimerCreate
	                   ( /* Just a text name, not used by the RTOS
	                     kernel. */
	                     "samplingTimer",
	                     /* The timer period in ticks, must be
	                     greater than 0. */
	                     MS2TICKS(SAMPLING_PERIOD_MS),
	                     /* The timers will auto-reload themselves
	                     when they expire. */
	                     pdTRUE,
	                     /* The ID is used to store a count of the
	                     number of times the timer has expired, which
	                     is initialised to 0. */
	                     ( void * ) 0,
	                     /* Each timer calls the same callback when
	                     it expires. */
						 IMUsamplingTimerCallback
	                   );

	osMessageQDef(imuQueue, 5, sizeof(uint8_t *));
	IMUQueueHandle = osMessageCreate(osMessageQ(imuQueue), NULL);


	Terminal_RegisterModule(&imuTerminalCommands_Handle);

	osThreadDef(IMUTask, IMUThread, osPriorityAboveNormal, 0, IMU_STACK_SIZE);
	IMUTaskHandle = osThreadCreate(osThread(IMUTask), NULL);

	return true;

}



void IMU_GPIO_Init(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
}

void IMU_Peripheral_Init(void)
{
	IMU_GPIO_Init();
	I2C1_Init();

}

void IMU_Gyr_Init(void)
{
	/*
	   *  Enable Block Data Update
	   */
	  lsm6dsl_block_data_update_set(&Gyroscope, PROPERTY_ENABLE);
	  /*
	   * Set Output Data Rate
	   */
	  lsm6dsl_gy_data_rate_set(&Gyroscope, LSM6DSL_GY_ODR_12Hz5);
	  /*
	   * Set full scale
	   */
	  lsm6dsl_gy_full_scale_set(&Gyroscope, LSM6DSL_2000dps);
	  /* Gyroscope - filtering chain */
	  lsm6dsl_gy_band_pass_set(&Gyroscope, LSM6DSL_HP_260mHz_LP1_STRONG);
}

void IMU_Acc_Init(void)
{
	  /*
	   *  Enable Block Data Update
	   */
	  lsm303agr_xl_block_data_update_set(&Accelerometer, PROPERTY_ENABLE);
	  /*
	   * Set Output Data Rate
	   */
	  lsm303agr_xl_data_rate_set(&Accelerometer, LSM303AGR_XL_ODR_10Hz);
	  /*
	   * Set accelerometer full scale
	   */
	  lsm303agr_xl_full_scale_set(&Accelerometer, LSM303AGR_2g);
	  /*
	   * Enable temperature sensor
	   */
	  lsm303agr_temperature_meas_set(&Accelerometer, LSM303AGR_TEMP_ENABLE);
	  /*
	   * Set device in continuos mode
	   */
	  lsm303agr_xl_operating_mode_set(&Accelerometer, LSM303AGR_HR_12bit);
}

void IMU_Mag_Init(void)
{
	/*
	*  Enable Block Data Update
	*/
	lsm303agr_mag_block_data_update_set(&Magnetometer, PROPERTY_ENABLE);
	/*
	* Set Output Data Rate
	*/
	lsm303agr_mag_data_rate_set(&Magnetometer, LSM303AGR_MG_ODR_10Hz);
	/*
	* Set / Reset magnetic sensor mode
	*/
	lsm303agr_mag_set_rst_mode_set(&Magnetometer, LSM303AGR_SENS_OFF_CANC_EVERY_ODR);
	/*
	* Enable temperature compensation on mag sensor
	*/
	lsm303agr_mag_offset_temp_comp_set(&Magnetometer, PROPERTY_ENABLE);
	/*
	* Set magnetometer in continuos mode
	*/
	lsm303agr_mag_operating_mode_set(&Magnetometer, LSM303AGR_CONTINUOUS_MODE);

}



void I2C1_Init(void)
{

	  hi2c1.Instance = I2C1;
	  hi2c1.Init.ClockSpeed = 100000;
	  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	  hi2c1.Init.OwnAddress1 = 0;
	  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	  hi2c1.Init.OwnAddress2 = 0;
	  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	  {
	    _Error_Handler(__FILE__, __LINE__);
	  }

	I2C_Semaphore = xSemaphoreCreateBinary();
	if (I2C_Semaphore != NULL)
	{
		xSemaphoreGive(I2C_Semaphore);
	}

}

void IMU_Gyr_read_mdps(int16_t *angular_rate_mdps)
{
	lsm6dsl_reg_t reg;
	lsm6dsl_status_reg_get(&Gyroscope, &reg.status_reg);
    if (reg.status_reg.gda)
    {
      /* Read angular rate data */
      memset(data_raw_angular_rate.u8bit, 0x00, 3*sizeof(int16_t));
      lsm6dsl_angular_rate_raw_get(&Gyroscope, data_raw_angular_rate.u8bit);
      angular_rate_mdps[0] = (int16_t)lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[0]);
      angular_rate_mdps[1] = (int16_t)lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[1]);
      angular_rate_mdps[2] = (int16_t)lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[2]);

      Terminal_Print("Angular rate \t[mdps]:\t%d\t%d\t%d",
              angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
    }
}

void IMU_ACC_read_mg(int16_t *acceleration_mg)
{
	lsm303agr_reg_t reg;
    lsm303agr_xl_status_get(&Accelerometer, &reg.status_reg_a);

    if (reg.status_reg_a.zyxda)
    {
      /* Read accelerometer data */
      memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
      lsm303agr_acceleration_raw_get(&Accelerometer, data_raw_acceleration.u8bit);
      acceleration_mg[0] = (int16_t)lsm303agr_from_fs_2g_hr_to_mg( data_raw_acceleration.i16bit[0]);
      acceleration_mg[1] = (int16_t)lsm303agr_from_fs_2g_hr_to_mg( data_raw_acceleration.i16bit[1] );
      acceleration_mg[2] = (int16_t)lsm303agr_from_fs_2g_hr_to_mg( data_raw_acceleration.i16bit[2] );

      Terminal_Print("Acceleration \t[mg]:\t\t%d\t%d\t%d",
              acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
    }
}

void IMU_Temp_read_deg(int16_t *temperature_degC)
{
	lsm303agr_reg_t reg;
    lsm303agr_temp_data_ready_get(&Accelerometer, &reg.byte);
    if (reg.byte)
    {
      /* Read temperature data */
      memset(data_raw_temperature.u8bit, 0x00, sizeof(int16_t));
      lsm303agr_temperature_raw_get(&Accelerometer, data_raw_temperature.u8bit);
      *temperature_degC = (int16_t)lsm303agr_from_lsb_hr_to_celsius( data_raw_temperature.i16bit);

      Terminal_Print("Temperature \t[degC]:\t%d\r\n", *temperature_degC);
    }
}

void IMU_Mag_read_mG(int16_t *magnetic_mG)
{
	lsm303agr_reg_t reg;
    lsm303agr_mag_status_get(&Magnetometer, &reg.status_reg_m);
    if (reg.status_reg_m.zyxda)
    {
      /* Read magnetic field data */
      memset(data_raw_magnetic.u8bit, 0x00, 3*sizeof(int16_t));
      lsm303agr_magnetic_raw_get(&Magnetometer, data_raw_magnetic.u8bit);
      magnetic_mG[0] = lsm303agr_from_lsb_to_mgauss( data_raw_magnetic.i16bit[0]);
      magnetic_mG[1] = lsm303agr_from_lsb_to_mgauss( data_raw_magnetic.i16bit[1]);
      magnetic_mG[2] = lsm303agr_from_lsb_to_mgauss( data_raw_magnetic.i16bit[2]);

      Terminal_Print("Magnetic field [mG]:\t\t%d\t%d\t%d",
              magnetic_mG[0], magnetic_mG[1], magnetic_mG[2]);
    }
}

static int32_t IMU_platform_write(void *handle, uint8_t Reg, uint8_t *Bufp,
                              uint16_t len)
{
	if (I2C_Semaphore == NULL) return -1;
	if( xSemaphoreTake(I2C_Semaphore, MS2TICKS(500)) == pdTRUE)
	{
		  uint32_t i2c_add = (uint32_t)handle;

		  if (i2c_add == LSM303AGR_I2C_ADD_XL)
		  {
			/* enable auto incremented in multiple read/write commands */
			Reg |= 0x80;
		  }

		  HAL_I2C_Mem_Write(&hi2c1, i2c_add, Reg,
							I2C_MEMADD_SIZE_8BIT, Bufp, len, 1000);

		  xSemaphoreGive(I2C_Semaphore);

		  return 0;
	}
	return -2;
}

static int32_t IMU_platform_read(void *handle, uint8_t Reg, uint8_t *Bufp,
                             uint16_t len)
{
  uint32_t i2c_add = (uint32_t)handle;
  if (i2c_add == LSM303AGR_I2C_ADD_XL)
  {
    /* enable auto incremented in multiple read/write commands */
    Reg |= 0x80;
  }
  HAL_I2C_Mem_Read(&hi2c1, (uint8_t) i2c_add, Reg,
                   I2C_MEMADD_SIZE_8BIT, Bufp, len, 1000);
  return 0;
}

uint32_t IMU_get_last_id_from_flash(void)
{
	tIMU_data last_data = {0};
	if(Flash_GetLastData(&last_data))
		return last_data.id;


	return IMU_INVALID_ID;
}

void IMU_dump_structure(tIMU_data *imuData)
{

    Terminal_Print("Angular rate \t[mdps]:\t%d\t%d\t%d",
                 imuData->angular_rate_mdps[0], imuData->angular_rate_mdps[1], imuData->angular_rate_mdps[2]);
    Terminal_Print("Acceleration \t[mg]:\t\t%d\t%d\t%d",
    		imuData->acceleration_mg[0], imuData->acceleration_mg[1], imuData->acceleration_mg[2]);
    Terminal_Print("Magnetic field [mG]:\t\t%d\t%d\t%d",
    		imuData->magnetic_mG[0], imuData->magnetic_mG[1], imuData->magnetic_mG[2]);
    Terminal_Print("Temperature \t[degC]:\t%d\r\n", imuData->temperature_degC);
}

void IMU_parse_cmd(uint8_t enumCase, char *subcommand)
{
	static tIMU_data imuData = {0};
	uint32_t number_of_items;
	switch (enumCase)
	{
	case DISPLAY_LAST_RECORD:

		if(Flash_GetLastData(&imuData))
		{
			Terminal_Print("Displaying the last record:");
			IMU_dump_structure(&imuData);
		}
		else
			Terminal_Print("No Entries were found");

		break;
	case DISPLAY_RECORDS:
		number_of_items = atoi(subcommand);
		if(number_of_items == 0)
		{
			Terminal_Print("SubCommand '%s' Must be a number and greater than zero", subcommand);
			break;
		}

		tFlash_data *pItemFromFlash = Flash_GetBeginningPointerForItems(number_of_items);

		if(number_of_items > IMU_data.id)
		{
			Terminal_Print("Requested number of records does not exist. Displaying the last %d IMU records if possible.", IMU_data.id);
			number_of_items = IMU_data.id;
			pItemFromFlash = Flash_GetBeginningPointerForItems(number_of_items);
		}

		if(Flash_ValidateData(*pItemFromFlash))
		{

			Terminal_Print("Displaying %d IMU records", number_of_items);
			for(int i = 0; i < IMU_data.id; i++)
			{
				if(Flash_ValidateData(*pItemFromFlash))
				{
					Terminal_Print("Displaying record #%d with id %d at address 0x%X", i+1, ((tIMU_data *)&pItemFromFlash->data)->id, pItemFromFlash);
					IMU_dump_structure((tIMU_data *)&pItemFromFlash->data);
					pItemFromFlash++;
				}
			}
		}
		else
			Terminal_Print("No Entries were found");
		break;
	default:
		break;
	}
}

void IMUThread(void const * argument)
{
	uint32_t last_id_from_flash = IMU_get_last_id_from_flash();
	if(last_id_from_flash == IMU_INVALID_ID)
		IMU_data.id = 0;
	else
		IMU_data.id = last_id_from_flash + 1;

	Terminal_Print("%d IMU Entries were found", IMU_data.id);

	uint32_t ulNotifiedValue;

	while (1)
	{
		xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
				0x00, 				/* Don't clear any notification bits on exit. */
                &ulNotifiedValue, 	/* Notified value pass out in ulNotifiedValue. */
                portMAX_DELAY); 	/* Block and wait forever until task is notified. */

		if (BIT_IS_SET(ulNotifiedValue, START_SAMPLING))
		{
			Terminal_Print("Button pressed! starting to sample...");

	    	if( xTimerStart( samplingTimer, MS2TICKS(SAMPLING_PERIOD_MS) ) != pdPASS )
			{
				/* The start command was not executed successfully.  Take appropriate
				action here. */
			}

		}

		if (BIT_IS_SET(ulNotifiedValue, SAMPLE_COMMAND))
		{
			Terminal_Print("Getting data from IMU And storing into the flash with id = %d", IMU_data.id);
			IMU_Gyr_read_mdps(IMU_data.angular_rate_mdps);
			IMU_ACC_read_mg(IMU_data.acceleration_mg);
			IMU_Mag_read_mG(IMU_data.magnetic_mG);
			IMU_Temp_read_deg(&((&IMU_data)->temperature_degC));
			if( Flash_WriteInBulk((uint8_t *)&IMU_data, sizeof(IMU_data)) == OK)
			{
				IMU_data.id++;
			}
		}


		if (BIT_IS_SET(ulNotifiedValue, STOP_SAMPLING))
		{
			if( xTimerStop( samplingTimer , SAMPLING_PERIOD_MS ) != pdPASS )
			{
				//Should not get here.

				/* The stop command was not executed successfully.  Take appropriate
				action here. */
			}
			Terminal_Print("Button pressed! stop sampling...");
		}

	}
}

void IMUsamplingTimerCallback(TimerHandle_t xTimer)
{
	uint32_t ulValue = SAMPLE_COMMAND;
	xTaskNotify(IMUTaskHandle, ulValue ,eSetValueWithOverwrite);
}

