/*
 * utility.h
 *
 *  Created on: 14Mar.,2020
 *      Author: Sahand Maleki
 */

#ifndef UTILITY_UTILITY_H_
#define UTILITY_UTILITY_H_

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOSConfig.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MS2TICKS(ms)	(ms * configTICK_RATE_HZ)/1000
#define BIT_IS_SET(first, second)	(first&second)==second? true:false
#define member_size(type, member) sizeof(((type *)0)->member)

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 	 __  _  _  __  _    _  ___    _  _ ____ ___ _  _  __   __   ___
// 	|__] |  | |__] |    | |       |\/| |___  |  |__| |  | |  \ [__
// 	|    |__| |__] |___ | |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
bool 		compareBits(uint32_t Param1, uint32_t Param2);
bool 		strEquals(char *first, char *second);
bool 		strEqualsWithSize(char *first, char *second, size_t size);
void 		replace_char(char* str, char find, char replace);
void 		replace_end_of_line_with_null(char* str);
uint64_t 	utilityAbs(int64_t value);
#endif /* UTILITY_UTILITY_H_ */
