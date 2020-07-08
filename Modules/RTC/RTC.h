/*
 * RTC.h
 *
 *  Created on: 10 March 2020
 *      Author: Sahand Maleki
 */

#ifndef RTC_RTC_H_
#define RTC_RTC_H_


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 	 __  _  _  __  _    _  ___    _  _ ____ ___ _  _  __   __   ___
// 	|__] |  | |__] |    | |       |\/| |___  |  |__| |  | |  \ [__
// 	|    |__| |__] |___ | |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void 		RTC_Init(void);
uint64_t 	RTC_GetTime_ms(void);
uint32_t 	RTC_GetRunTime_ms(void);

#endif /* RTC_RTC_H_ */
