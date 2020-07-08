/*
 * Flash.h
 *
 *  Created on: 13Mar.,2020
 *      Author: Sahand Maleki
 */

#ifndef FLASH_H_
#define FLASH_H_

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__ 
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "stm32f4xx_hal.h"
#include "Sensors/IMU/IMU.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__ 
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//			
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define FLASH_ADDRESS_BEGIN 	0x08040000
#define FLASH_ADDRESS_MIDDLE	0x0805FFFF
#define FLASH_ADDRESS_END		0x0807FFFF
#define UNINITIALIZED_DATA		0xFFFFFFFF
#define FLASH_INVALID_ID 		0xFFFFFFFF

#define BYTE 					FLASH_TYPEPROGRAM_BYTE
#define HALFWORD				FLASH_TYPEPROGRAM_HALFWORD
#define WORD					FLASH_TYPEPROGRAM_WORD
#define DOUBLEWORD				FLASH_TYPEPROGRAM_DOUBLEWORD

#define BYTESIZE				1
#define HALFWORDSIZE 			2 * BYTESIZE
#define WORDSIZE				4 * BYTESIZE
#define	DOUBLEWORDSIZE			8 * BYTESIZE

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 		 __  _  _  __  _    _  ___    ___ _   _  __  ____  ___
// 		|__] |  | |__] |    | |        |   \_/  |__] |___ [__ 
// 		|    |__| |__] |___ | |___     |    |   |    |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
typedef enum
{
	UNLOCK_FLASH_CONTROL_REGISTER = 0,
	LOCK_FLASH_CONTROL_REGISTER,
	UNLOCK_FLASH_CONTROL_OPTION_REGISTERS,
	LOCK_FLASH_CONTROL_OPTION_REGISTERS,
	OK = 0xFF

}teFlash_Failures;

typedef struct{
	uint32_t 	id;
	uint32_t 	data[6];
	uint32_t	crc32;
}tFlash_data;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 	 __  _  _  __  _    _  ___    _  _ ____ ___ _  _  __   __   ___
// 	|__] |  | |__] |    | |       |\/| |___  |  |__| |  | |  \ [__ 
// 	|    |__| |__] |___ | |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void 				Flash_Init(void);
uint8_t 			Flash_WriteInBulk(uint8_t *Data, uint16_t sizeInBytes);
uint32_t 			Flash_GetCurrentAddress_u32(void);
bool 				Flash_ValidateData(tFlash_data flashData);
bool 				Flash_GetLastData(void *data);
tFlash_data*		Flash_GetBeginningPointerForItems(uint32_t number_of_items);


#endif /* FLASH_H_ */
