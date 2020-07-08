/*
 * Flash.c
 *
 *  Created on: 13Mar.,2020
 *      Author: Sahand Maleki
 */

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__ 
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "Flash.h"
#include "Utility/utility.h"
#include "Terminal/terminal.h"
#include "CRC/crc.h"
#include "RTC/RTC.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//				_  _ __   __  _ __   __  _    ____  ___
//				|  ||__| |__/ ||__| |__] |    |___ [__
//				 \/ |  | |  \ ||  | |__] |___ |___ ___]
//				
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static teFlash_Failures		gFlashFailureFlags;
static uint32_t 			gFlashCurrentAddress = FLASH_ADDRESS_BEGIN;
static uint32_t 			gFlashLastId		 = 0;


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  __   __  _ _  _ __  ___ ____    _  _ ____ ___ _  _  __   __   ___
// |__] |__/ | |  ||__|  |  |___    |\/| |___  |  |__| |  | |  \ [__ 
// |    |  \ |  \/ |  |  |  |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static uint8_t				Flash_Write(uint32_t TypeProgram, uint64_t Data);
static void 				Flash_GetSizeOfTypes(uint16_t size, uint8_t *numberOfWords, uint8_t *numberOfHalfWords, uint8_t *numberOfBytes);
static uint8_t 				Flash_WriteMultipleSizes(uint8_t *Data, uint8_t numberOfWords, uint8_t numberOfHalfWords, uint8_t numberOfBytes);
static uint8_t	 			Flash_EraseSector(uint32_t sector);
static uint32_t 			Flash_CalculateAddressRollOverIfNecessary(uint32_t TypeProgram);
static void 				Flash_FindCurrentAddress(uint32_t timeout_ms);


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// _  _ ____ ___ _  _  __   __     _ _  _  __  _    ____ _  _ ____ _  _ ___ __  ___ _  __  _  _  ___
// |\/| |___  |  |__| |  | |  \    | |\/| |__] |    |___ |\/| |___ |\ |  | |__|  |  | |  | |\ | [__ 
// |  | |___  |  |  | |__| |__/    | |  | |    |___ |___ |  | |___ | \|  | |  |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//========================================================================
//Method: 			Flash_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
//Purpose:			Initialize the Flash Component
//operations:		Finding the latest address to be used in the application.
//Params:			N/P
//Returns:			N/A 
//------------------------------------------------------------------------
void Flash_Init(void)
{
	Flash_FindCurrentAddress(10000);
}

//========================================================================
//Method: 			Flash_GetCurrentAddress_u32
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
//Purpose:			Return the latest address used by flash
//operations:		-
//Params:			N/P
//Returns:			uint32_t latest address used by flash 
//------------------------------------------------------------------------
uint32_t Flash_GetCurrentAddress_u32(void)
{
	return gFlashCurrentAddress;
}

//========================================================================
//Method: 			Flash_GetSizeOfTypes
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Get different sizes that has created the general size
//operations:		finds the number of biggest size and moves on to the 
//					the smaller size and repeats the operation.
//Params:			
//					@size: 					General size to be converted to 
//											standard sizes. 
//					@numberOfWords:			pointer for the number of words
//											to be stored into
//					@numberOfHalfWords:		pointer for the number of 
//											Half Words to be stored into
//					@numberOfBytes:			pointer for the number of Bytest
//											to be stored into
//Returns:			N/A
//------------------------------------------------------------------------
void Flash_GetSizeOfTypes(uint16_t size, uint8_t *numberOfWords, uint8_t *numberOfHalfWords, uint8_t *numberOfBytes)
{
	uint16_t leftOfSize = size;

	*numberOfWords = leftOfSize/WORDSIZE;
	leftOfSize %= WORDSIZE;

	*numberOfHalfWords = leftOfSize/HALFWORDSIZE;
	leftOfSize %= HALFWORDSIZE;

	*numberOfBytes = leftOfSize/BYTESIZE;

}

//========================================================================
//Method: 			Flash_WriteInBulk
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Write data to the internal flash memory with id and CRC
//operations:		-Increments the flash Id
//					-Calculates the CRC of the data
//					-Store the data along with Flash Id and CRC
//Params:			
//					@data: 			Data that needs to be written to the flash 
//					@sizeInBytes:	size of the data in bytes
//Returns:			status of the operation. OK if everything was alright.
//------------------------------------------------------------------------
uint8_t Flash_WriteInBulk(uint8_t *data, uint16_t sizeInBytes)
{
	tFlash_data Data;
	uint32_t 	CalculatedCRC = 0;
	if(sizeInBytes < BYTESIZE) return -1;	//return if the size less than 1
	if(sizeInBytes > sizeof(Data.data)) return -2;
	memset(&Data.data, 0, sizeof(Data.data));
	memcpy(&Data.data, data, sizeInBytes);
	crc32_ieee802_3_stream_check(&CalculatedCRC, (uint8_t *)&(Data.data), sizeof(Data.data));
	Data.crc32 = CalculatedCRC;
	Data.id = gFlashLastId++;



	uint8_t numberOfWords = 0;
	uint8_t numberOfHalfWords = 0;
	uint8_t numberOfBytes = 0;

	//we are writing fixed size structs into flash. no need to worry if we are out of room in the end of flash.
	Flash_GetSizeOfTypes(sizeof(tFlash_data), &numberOfWords, &numberOfHalfWords, &numberOfBytes);
	Flash_WriteMultipleSizes((uint8_t *)&Data, numberOfWords, numberOfHalfWords, numberOfBytes);

	return OK;
}

//========================================================================
//Method: 			Flash_WriteMultipleSizes
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Write raw information to the internal flash memory in 
//					different sizes
//operations:		-Writes the words first
//					-Writes the halfwords
//					-Writes the bytes
//Params:			
//					@Data: 					pointer to the raw data that 
//											needs to be written into flash
//					@numberOfWords:			number of words in data
//					@numberOfHalfWords:		number of half words in data
//					@numberOfBytes:			number of bytes in data
//Returns:			status of the operation. OK if everything was alright.
//------------------------------------------------------------------------
uint8_t Flash_WriteMultipleSizes(uint8_t *Data, uint8_t numberOfWords, uint8_t numberOfHalfWords, uint8_t numberOfBytes)
{
	uint16_t usableDataIndex = 0;

	if(numberOfWords > 0)
	{
		for(int i = usableDataIndex; i < numberOfWords; i++)
		{
			uint32_t word = ((Data[usableDataIndex + 4*i + 3] << 24) | (Data[usableDataIndex + 4*i + 2] << 16) |
					(Data[usableDataIndex + 4*i + 1] << 8) | Data[usableDataIndex + 4*i]);


			Flash_Write(WORD, (uint32_t)word);
		}
		usableDataIndex += numberOfWords * WORDSIZE;
	}

	if(numberOfHalfWords > 0)
	{
		for(int i = 0; i < numberOfHalfWords; i++)
		{
			uint16_t halfWord = ((Data[usableDataIndex + 2*i + 1] << 8) | (Data[usableDataIndex + 2*i]));
			Flash_Write(HALFWORD, (uint16_t)halfWord);
		}
		usableDataIndex += numberOfHalfWords * HALFWORDSIZE;
	}

	if(numberOfBytes > 0)
	{
		for(int i = 0; i < numberOfBytes; i++)
		{
			uint8_t byte = Data[usableDataIndex + i];
			Flash_Write(BYTE, (uint8_t)byte);
		}
	}

	return OK;
}

//========================================================================
//Method: 			Flash_EraseSector
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Erase the flash sector specified. 
//					different sizes
//operations:		-Unlocks the flash
//					-Erases the flash
//					-locks the flash
//Params:			
//					@sector: 	sector to be erased. Value can be chosen
//								by appending the number of flash sector 
//								to the end of: 'FLASH_SECTOR_'
//Returns:			status of the operation. OK if everything was alright.
//------------------------------------------------------------------------
uint8_t Flash_EraseSector(uint32_t sector)
{
	uint32_t flashSectorError;
	FLASH_EraseInitTypeDef eraseInitStruct =
												{
													.TypeErase = FLASH_TYPEERASE_SECTORS,
													.NbSectors = 1,
													.Sector = sector
												};

	if(HAL_FLASH_Unlock() != HAL_OK)
	{
		gFlashFailureFlags |= UNLOCK_FLASH_CONTROL_REGISTER;
		return gFlashFailureFlags;
	}

	if(HAL_FLASH_OB_Unlock() != HAL_OK)
	{
		gFlashFailureFlags |= UNLOCK_FLASH_CONTROL_OPTION_REGISTERS;
		return gFlashFailureFlags;
	}

	HAL_FLASHEx_Erase(&eraseInitStruct, &flashSectorError);

	if(HAL_FLASH_Lock() != HAL_OK)
	{
		gFlashFailureFlags |= UNLOCK_FLASH_CONTROL_REGISTER;
		return gFlashFailureFlags;
	}

	if(HAL_FLASH_OB_Lock() != HAL_OK)
	{
		gFlashFailureFlags |= UNLOCK_FLASH_CONTROL_OPTION_REGISTERS;
		return gFlashFailureFlags;
	}

	return OK;
}

//========================================================================
//Method: 			Flash_Write
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Writes to the flash based on the type of data 
//operations:		-determines if rollover is necessary 
//					-if rollover is necessary, it takes care of it.
//					-Writes data to the flash
//					-Calculates the address after writing the data and
//					 sets the current address accordingly 
//Params:			
//					@programType: 	SizeType that can be chosen from:
//									BYTE, HALFWORD, WORD
//					@Data:			Data to be written to the flash
//Returns:			status of the operation. OK if everything was alright.
//------------------------------------------------------------------------
uint8_t Flash_Write(uint32_t programType, uint64_t Data)
{
	uint32_t flashAddress = Flash_CalculateAddressRollOverIfNecessary(programType);
	if(HAL_FLASH_Unlock() != HAL_OK)
	{
		gFlashFailureFlags |= UNLOCK_FLASH_CONTROL_REGISTER;
		return gFlashFailureFlags;
	}

	if(HAL_FLASH_OB_Unlock() != HAL_OK)
	{
		gFlashFailureFlags |= UNLOCK_FLASH_CONTROL_OPTION_REGISTERS;
		return gFlashFailureFlags;
	}
	HAL_FLASH_Program(programType , gFlashCurrentAddress, Data);
	gFlashCurrentAddress = flashAddress;

	if(HAL_FLASH_Lock() != HAL_OK)
	{
		gFlashFailureFlags |= LOCK_FLASH_CONTROL_REGISTER;
		return gFlashFailureFlags;
	}

	if(HAL_FLASH_OB_Lock() != HAL_OK)
	{
		gFlashFailureFlags |= LOCK_FLASH_CONTROL_OPTION_REGISTERS;
		return gFlashFailureFlags;
	}

	return OK;
}

//========================================================================
//Method: 			Flash_CalculateAddressRollOverIfNecessary
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//known bugs:		If total data is not word aligned there is going to be
//					a serious problem after the rollover.
//------------------------------------------------------------------------
//Purpose:			Taking care of Flash Rollover and Calculating the
//					address to be set to the current flash address
//operations:		-determines if rollover is necessary 
//					-if rollover is necessary, it takes care of it.
//					-Writes data to the flash
//					-Calculates the address after writing the data and
//					 sets the current address accordingly 
//Params:			
//					@programType: 	SizeType that can be chosen from:
//									BYTE, HALFWORD, WORD
//					@Data:			Data to be written to the flash
//Returns:			Future flash address
//------------------------------------------------------------------------
uint32_t Flash_CalculateAddressRollOverIfNecessary(uint32_t programType)
{
	uint32_t sizeToBeAdded = 0;
	switch(programType)
	{
	case BYTE:
		sizeToBeAdded = BYTESIZE;
		break;

	case HALFWORD:
		sizeToBeAdded = HALFWORDSIZE;
		break;

	case WORD:
		sizeToBeAdded = WORDSIZE;
		break;
	}

	
	if (gFlashCurrentAddress +  sizeToBeAdded> FLASH_ADDRESS_END)
	{
		Terminal_Print("Erasing sector %d of the Flash", FLASH_SECTOR_6);
		gFlashCurrentAddress = FLASH_ADDRESS_BEGIN;
		Flash_EraseSector(FLASH_SECTOR_6);
	}

	if ((gFlashCurrentAddress +  sizeToBeAdded > FLASH_ADDRESS_MIDDLE) && (gFlashCurrentAddress <= FLASH_ADDRESS_MIDDLE))
	{
		Terminal_Print("Erasing sector %d of the Flash", FLASH_SECTOR_7);
		gFlashCurrentAddress = FLASH_ADDRESS_MIDDLE + 1;
		Flash_EraseSector(FLASH_SECTOR_7);
	}

	return gFlashCurrentAddress +  sizeToBeAdded;
}

//========================================================================
//Method: 			Flash_ValidateData
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Validate the data stored into flash
//operations:		-Get the CRC stored in Flash
//					-Calculate CRC of the data stored in the flash
//					-Compare the CRCs. if equal then data is validated. 
//Params:			@timeout_ms:	Give up searching for the address
//									after the timeout in ms
//Returns:			bool: ture if data is validated.
//------------------------------------------------------------------------
bool Flash_ValidateData(tFlash_data flashData)
{
	uint32_t calculated_crc32 = 0;
	crc32_ieee802_3_stream_check(&calculated_crc32, (uint8_t *)&(flashData.data), sizeof(flashData.data));
	if(flashData.crc32 == calculated_crc32)
	{
		return true;
	}
	return false;
}


//========================================================================
//Method: 			Flash_FindCurrentAddress
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Finds the current address of the Flash
//operations:		-Finds the active sector by the greater id stored
//					-Finds the address in the sector by a binary search 
//					and validating the data before the current address
//Params:			@timeout_ms:	Give up searching for the address
//									after the timeout in ms
//Returns:			N/A
//------------------------------------------------------------------------
void Flash_FindCurrentAddress(uint32_t timeout_ms)
{
	bool flashAddressFound = false;
	uint32_t id_in_sector6 = 0;
	uint32_t id_in_sector7 = 0;
	bool	 search_sector6 = true;
	uint32_t *uninitializedAddress;
	uint32_t *initializedAddress;
	uint32_t *address = (uint32_t *)((FLASH_ADDRESS_END - FLASH_ADDRESS_BEGIN)/2 + FLASH_ADDRESS_BEGIN +1);
	uint32_t volatile numberOfSteps = 0;
	// determine which sector to search

	if(Flash_ValidateData(*((tFlash_data *)FLASH_ADDRESS_BEGIN)))
		id_in_sector6 = ((tFlash_data *)(FLASH_ADDRESS_BEGIN))->id;
	else
		id_in_sector6 = 0xffffffff;

	if (Flash_ValidateData(*((tFlash_data *)((FLASH_ADDRESS_END - FLASH_ADDRESS_BEGIN)/2 + FLASH_ADDRESS_BEGIN +1))))
		id_in_sector7 = ((tFlash_data *)((FLASH_ADDRESS_END - FLASH_ADDRESS_BEGIN)/2 + FLASH_ADDRESS_BEGIN +1))->id;
	else
		id_in_sector7 = 0xffffffff;
	if(id_in_sector6 == id_in_sector7 && id_in_sector6 == FLASH_INVALID_ID)
	{
		gFlashCurrentAddress = FLASH_ADDRESS_BEGIN;
		flashAddressFound = true;
		Terminal_Print("Found the flash address at 0x%X in %d steps", gFlashCurrentAddress, numberOfSteps);
	}
	else if((id_in_sector6 != FLASH_INVALID_ID && id_in_sector7 == FLASH_INVALID_ID) ||
			(id_in_sector6 > id_in_sector7))
		search_sector6 = true;
	else if((id_in_sector6 == FLASH_INVALID_ID && id_in_sector7 != FLASH_INVALID_ID) ||
			(id_in_sector6 < id_in_sector7))
		search_sector6 = false;


	if(search_sector6)
	{
		uninitializedAddress = address;
		initializedAddress = (uint32_t *)FLASH_ADDRESS_BEGIN - 4;
	}
	else
	{
		uninitializedAddress = (uint32_t *)FLASH_ADDRESS_END;
		initializedAddress = address;

	}
	address = utilityAbs(uninitializedAddress - initializedAddress)/2 + initializedAddress;
	numberOfSteps++;
	//avoid doing it recursively
	uint64_t timeout = timeout_ms + RTC_GetTime_ms();
	while (!flashAddressFound)
	{
		if(RTC_GetTime_ms() > timeout)
		{
			Terminal_Print("Gave up trying to find the flash address.");
			return;
		}
		numberOfSteps++;
		if(*address == UNINITIALIZED_DATA)
		{
			uninitializedAddress = address;

		}
		else
		{
			initializedAddress = address;
		}
		address = utilityAbs(uninitializedAddress - initializedAddress)/2 + initializedAddress;

		if((uninitializedAddress - initializedAddress) == 1)
		{
			gFlashCurrentAddress = (uint32_t)uninitializedAddress;
			gFlashLastId = ((tFlash_data *)(gFlashCurrentAddress - sizeof(tFlash_data)))->id;
			if(Flash_ValidateData(*((tFlash_data *)(gFlashCurrentAddress - sizeof(tFlash_data)))) ||
				gFlashCurrentAddress == FLASH_ADDRESS_BEGIN)
			{

				flashAddressFound = true;
				Terminal_Print("Found the flash address at 0x%X in %d steps", gFlashCurrentAddress, numberOfSteps);
			}

		}
	}
}

//========================================================================
//Method: 			Flash_GetLastData
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Get the last record stored into flash
//operations:		-Get the last record from the flash
//					-Validate the data
//Params:			@data:	pointer for the record to be copied into.
//							If data is not validated, pointer is going to
//							be NULL
//Returns:			bool: true if last found record is validated. 
//------------------------------------------------------------------------
bool Flash_GetLastData(void *data)
{
	//if address is before the FLASH_ADDRESS_BEGIN
	if(gFlashCurrentAddress - sizeof(tFlash_data) >= FLASH_ADDRESS_BEGIN)
	{
		if(Flash_ValidateData(*((tFlash_data *)(gFlashCurrentAddress - sizeof(tFlash_data)))))
		{
			Terminal_Print("Last Record was found at address [0x%08X]", gFlashCurrentAddress);
			memcpy(data, &((tFlash_data *)(gFlashCurrentAddress - sizeof(tFlash_data)))->data, sizeof(((tFlash_data *)FLASH_ADDRESS_BEGIN)->data));
			return true;
		}
	}
	else if(*(uint32_t *)(FLASH_ADDRESS_END - (FLASH_ADDRESS_BEGIN - (gFlashCurrentAddress - sizeof(tFlash_data)))) != UNINITIALIZED_DATA)
	{
		if(Flash_ValidateData(*((tFlash_data *)(FLASH_ADDRESS_END - (FLASH_ADDRESS_BEGIN - (gFlashCurrentAddress - sizeof(tFlash_data)))))))
		{
			Terminal_Print("Last Record was found at address [0x%08X]", gFlashCurrentAddress);
			memcpy(data, &((tFlash_data *)(FLASH_ADDRESS_END - (FLASH_ADDRESS_BEGIN - (gFlashCurrentAddress - sizeof(tFlash_data)))))->data, sizeof(((tFlash_data *)FLASH_ADDRESS_BEGIN)->data));
			return true;
		}
	}
	else if (*(uint32_t *)(FLASH_ADDRESS_BEGIN) != UNINITIALIZED_DATA)
	{
		if(Flash_ValidateData(*((tFlash_data *)FLASH_ADDRESS_BEGIN)))
		{
			Terminal_Print("Last Record was found at address [0x%08X]", gFlashCurrentAddress);
			memcpy(data, &((tFlash_data *)FLASH_ADDRESS_BEGIN)->data, sizeof(((tFlash_data *)FLASH_ADDRESS_BEGIN)->data));
			return true;
		}
	}

	data = ((tFlash_data *)NULL);
	return false;
}

//========================================================================
//Method: 			Flash_GetBeginningPointerForItems
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Get the pointer from the beginning of the items based
//					on the number of items. 
//operations:		-Get the beginning record from the flash
//					-Validate the data
//Params:			@number_of_items:	from the current flash address, 
//										the number of records to go back 
//										and get the pointer from
//Returns:			tFlash_data *: 		Pointer to the beginning record. 
//										NULL if beginning data is not validated.  
//------------------------------------------------------------------------
tFlash_data *Flash_GetBeginningPointerForItems(uint32_t number_of_items)
{
	tFlash_data *pFirstItemFromFlash;
	//if address is before the
	if(Flash_GetCurrentAddress_u32() - sizeof(tIMU_data)*number_of_items >= FLASH_ADDRESS_BEGIN)
	{
		pFirstItemFromFlash = ((tFlash_data *)(gFlashCurrentAddress - sizeof(tFlash_data)*number_of_items));
		if(Flash_ValidateData(*((tFlash_data *)(gFlashCurrentAddress - sizeof(tFlash_data)*number_of_items))))
			return pFirstItemFromFlash;
	}
	else if(*(uint32_t *)(FLASH_ADDRESS_END - (FLASH_ADDRESS_BEGIN - (Flash_GetCurrentAddress_u32() - sizeof(tIMU_data)*number_of_items))) != UNINITIALIZED_DATA)
	{
		pFirstItemFromFlash = ((tFlash_data *)(FLASH_ADDRESS_END - (FLASH_ADDRESS_BEGIN - (gFlashCurrentAddress - sizeof(tFlash_data)*number_of_items))));
		if(Flash_ValidateData(*((tFlash_data *)(FLASH_ADDRESS_END - (FLASH_ADDRESS_BEGIN - (gFlashCurrentAddress - sizeof(tFlash_data)*number_of_items))))))
			return pFirstItemFromFlash;
	}
	else if (*(uint32_t *)(FLASH_ADDRESS_BEGIN) != UNINITIALIZED_DATA)
	{

		pFirstItemFromFlash = (tFlash_data *)FLASH_ADDRESS_BEGIN;
		if(Flash_ValidateData(*((tFlash_data *)FLASH_ADDRESS_BEGIN)))
			return pFirstItemFromFlash;
	}
	else
	{
		pFirstItemFromFlash = ((tFlash_data *)NULL);
	}

	return pFirstItemFromFlash;
}




