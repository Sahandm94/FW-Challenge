/*
 * utility.c
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
#include "utility.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// _  _ ____ ___ _  _  __   __     _ _  _  __  _    ____ _  _ ____ _  _ ___ __  ___ _  __  _  _  ___
// |\/| |___  |  |__| |  | |  \    | |\/| |__] |    |___ |\/| |___ |\ |  | |__|  |  | |  | |\ | [__
// |  | |___  |  |  | |__| |__/    | |  | |    |___ |___ |  | |___ | \|  | |  |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool compareBit(uint32_t Param1, uint32_t Bit)
{
	return ((Param1 & Bit) == Bit) ? true : false;
}

bool strEquals(char *first, char *second)
{
	return strcmp(first, second)?  false: true;
}

bool strEqualsWithSize(char *first, char *second, size_t size)
{
	return strncmp(first, second, size)?  false: true;
}

void replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
}

void replace_end_of_line_with_null(char* str){
    char *current_pos = strstr(str, "\r\n");
    if(!current_pos)
    {
    	current_pos = strstr(str, "\r");
        if(!current_pos)
        {
        	current_pos = strstr(str, "\n");
            if(!current_pos)
            {
            	 *current_pos = '\0';
            }
        }
        else
        {
            *current_pos = '\0';
        }
    }
    else
    {
        *current_pos = '\0';
    }
}

uint64_t utilityAbs(int64_t value)
{
	if(value >= 0)
		return value;
	else
		return -value;
}
