/*
 * RTC.c
 *
 *  Created on: 10 March 2020
 *      Author: Sahand Maleki
 */

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rtc.h"
#include "RTC.h"
#include "Terminal/terminal.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define RTC_MODULE_NAME "RTC"
#define SUBSECONDS2MS(subSeconds)

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  __   __  _ _  _ __  ___ ____    _  _ ____ ___ _  _  __   __   ___
// |__] |__/ | |  ||__|  |  |___    |\/| |___  |  |__| |  | |  \ [__
// |    |  \ |  \/ |  |  |  |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static void 	RTC_ParseCmd(uint8_t enumCase, char *SubCommands);
static char * 	RTC_GetWeekDayString(uint8_t weekday);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//				_  _ __   __  _ __   __  _    ____  ___
//				|  ||__| |__/ ||__| |__] |    |___ [__
//				 \/ |  | |  \ ||  | |__] |___ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
RTC_HandleTypeDef 			hrtc;
static uint64_t 			startTime_ms;
static char 				WeekdayNames_Strings[7][10] = { "Monday", "Tuesday","Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
tTerminal_ModuleCmdHandle 	RTCTerminal_Commands_handle = { .commands = {"get_time\0" },
															.name = RTC_MODULE_NAME,
															.moduleParseCmdFunction = RTC_ParseCmd };

enum {
	GET_TIME = 0,
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// _  _ ____ ___ _  _  __   __     _ _  _  __  _    ____ _  _ ____ _  _ ___ __  ___ _  __  _  _  ___
// |\/| |___  |  |__| |  | |  \    | |\/| |__] |    |___ |\/| |___ |\ |  | |__|  |  | |  | |\ | [__
// |  | |___  |  |  | |__| |__/    | |  | |    |___ |___ |  | |___ | \|  | |  |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//========================================================================
//Method: 			RTC_Register_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void RTC_Register_Init(void) {

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	RTC_AlarmTypeDef sAlarm;

	/**Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2) {
		hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
		hrtc.Init.AsynchPrediv = 127;
		hrtc.Init.SynchPrediv = 999;
		hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
		hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
		hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
		if (HAL_RTC_Init(&hrtc) != HAL_OK) {
			_Error_Handler(__FILE__, __LINE__);
		}

		/**Initialize RTC and set the Time and Date
		 */
		sTime.Hours = 0x0;
		sTime.Minutes = 0x0;
		sTime.Seconds = 0x0;
		sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		sTime.StoreOperation = RTC_STOREOPERATION_RESET;
		if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
			_Error_Handler(__FILE__, __LINE__);
		}

		sDate.WeekDay = RTC_WEEKDAY_MONDAY;
		sDate.Month = RTC_MONTH_JANUARY;
		sDate.Date = 0x1;
		sDate.Year = 0x0;

		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
			_Error_Handler(__FILE__, __LINE__);
		}

		startTime_ms = RTC_GetTime_ms();

		/**Enable the Alarm A
		 */
		sAlarm.AlarmTime.Hours = 0x0;
		sAlarm.AlarmTime.Minutes = 0x0;
		sAlarm.AlarmTime.Seconds = 0x0;
		sAlarm.AlarmTime.SubSeconds = 0x0;
		sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
		sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
		sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
		sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
		sAlarm.AlarmDateWeekDay = 0x1;
		sAlarm.Alarm = RTC_ALARM_A;
		if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK) {
			_Error_Handler(__FILE__, __LINE__);
		}

		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);
	} else {
		startTime_ms = RTC_GetTime_ms();
	}

}

//========================================================================
//Method: 			RTC_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void RTC_Init(void) {
	RTC_Register_Init();
	Terminal_RegisterModule(&RTCTerminal_Commands_handle);
}

//========================================================================
//Method: 			RTC_GetTime_ms
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
uint64_t RTC_GetTime_ms(void) {
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	return (sDate.Year * 31556926 + sDate.Month * 2629743 + sDate.Date * 86400
			+ sTime.Hours * 3600 + sTime.Minutes * 60 + sTime.Seconds) * 1000
			+ 1000 - (sTime.SubSeconds * 1000) / (sTime.SecondFraction + 1);

}

//========================================================================
//Method: 			RTC_GetRunTime_ms
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
uint32_t RTC_GetRunTime_ms(void) {
	uint32_t time_ms = RTC_GetTime_ms() - startTime_ms;
	return time_ms;
}

//========================================================================
//Method: 			RTC_ParseCmd
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
static void RTC_ParseCmd(uint8_t enumCase, char *SubCommands) {
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	switch (enumCase) {
	case GET_TIME:
		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
		Terminal_Print("Time is : %s %04d-%02d-%02d %02d:%02d:%02d.%03d",
				RTC_GetWeekDayString(sDate.WeekDay), sDate.Year, sDate.Month,
				sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds,
				999 - (sTime.SubSeconds * 1000) / (sTime.SecondFraction + 1));
		Terminal_Print("Runtime is : %d", RTC_GetRunTime_ms());
		break;
	default:
		break;
	}

}

//========================================================================
//Method: 			RTC_GetWeekDayString
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
static char *RTC_GetWeekDayString(uint8_t weekday) {
	switch (weekday) {
	case RTC_WEEKDAY_MONDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_MONDAY - 1];
	case RTC_WEEKDAY_TUESDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_TUESDAY - 1];
	case RTC_WEEKDAY_WEDNESDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_WEDNESDAY - 1];
	case RTC_WEEKDAY_THURSDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_THURSDAY - 1];
	case RTC_WEEKDAY_FRIDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_FRIDAY - 1];
	case RTC_WEEKDAY_SATURDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_SATURDAY - 1];
	case RTC_WEEKDAY_SUNDAY:
		return WeekdayNames_Strings[RTC_WEEKDAY_SUNDAY - 1];
	default:
		return '\0';
		break;

	}
}
