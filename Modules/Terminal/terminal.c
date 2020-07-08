/*
 * terminal.c
 *
 *  Created on: 3Jan.,2020
 *      Author: Sahand Maleki
 */

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <string.h>
#include "terminal.h"
#include "RTC/RTC.h"

#if PROJECT_TERMINAL_ENABLED

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define TERMINAL_MODULE_NAME "TERMINAL"

enum{
	HELP = 0,
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  __   __  _ _  _ __  ___ ____    _  _ ____ ___ _  _  __   __   ___
// |__] |__/ | |  ||__|  |  |___    |\/| |___  |  |__| |  | |  \ [__
// |    |  \ |  \/ |  |  |  |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static void 	Terminal_DMA_Init(void);
static void 	Terminal_GPIO_Init(void);
static void 	Terminal_UART_Init(void);
static void 	TerminalThread(void const * argument);
static void 	Terminal_RxMessageHalfCompletedCallBack(UART_HandleTypeDef *huart);
static void 	Terminal_RxMessageCompletedCallBack(UART_HandleTypeDef *huart);
static void 	Terminal_ProcessRxBuffer(void);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//				_  _ __   __  _ __   __  _    ____  ___
//				|  ||__| |__/ ||__| |__] |    |___ [__
//				 \/ |  | |  \ ||  | |__] |___ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static tTerminal_ModuleCmdHandle *pRegisteredModules_hadnles[MAX_TERMINAL_REGISTERED_MODULES];
static tTerminal_ModuleCmdHandle terminalTerminalCommands_Handle = {.commands = {"help\0"},
																	.name = TERMINAL_MODULE_NAME,
																	.moduleParseCmdFunction = Terminal_ParseCommand};
static uint8_t				 numberOfRegisteredModules = 0;

//buffers for RX transmission
static uint8_t rxData_buffer[RX_BUFFER_SIZE];
static uint8_t rxData[RX_CACHED_DATA_SIZE];

static tUART_message *pQueueReceiveMessage;
static tUART_message *pQueueSendMessage;

//semaphore for the terminal rx
static SemaphoreHandle_t Terminal_rx_Semaphore;

static tURAT_RX_transmission rx_transmission = { .rx_data_buffer_last_position = 0,
		.rx_data_buffer_current_position = 0,
		.cached_data_buffer_current_position = 0, .rx_data_size = 0,
		.data_complete = false,
		.idle_line_should_be_checked = true};

static char printBuffer[200];



//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// _  _ ____ ___ _  _  __   __     _ _  _  __  _    ____ _  _ ____ _  _ ___ __  ___ _  __  _  _  ___
// |\/| |___  |  |__| |  | |  \    | |\/| |__] |    |___ |\/| |___ |\ |  | |__|  |  | |  | |\ | [__
// |  | |___  |  |  | |__| |__/    | |  | |    |___ |___ |  | |___ | \|  | |  |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//========================================================================
//Method: 			Terminal_DMA_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Initialize DMA for Terminal
//operations:		
//Params:			
//Returns:			
//------------------------------------------------------------------------
static void Terminal_DMA_Init(void) {
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

//========================================================================
//Method: 			Terminal_DMA_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Initialize DMA for Terminal
//operations:		UART_Init() Function that gets called from this function 
//					needs to be changed based on the hardware choice
//Params:			-
//Returns:			-
//------------------------------------------------------------------------
void Terminal_Init(void) {
	Terminal_GPIO_Init();
	Terminal_DMA_Init();
	Terminal_UART_Init();

	/* Create the TerminalThread*/
	/* definition and creation of terminalTask */
	osThreadDef(terminalTask, TerminalThread, osPriorityNormal, 0,
			TERMINAL_TASK_STACK_SIZE);
	terminalTaskHandle = osThreadCreate(osThread(terminalTask), NULL);

	osMessageQDef(rxTxQueue, 5, sizeof(tUART_message *));
	terminalRxQueueHandle = osMessageCreate(osMessageQ(rxTxQueue), NULL);

	Terminal_rx_Semaphore = xSemaphoreCreateBinary();
	if (Terminal_rx_Semaphore != NULL)
	{
		xSemaphoreGive(Terminal_rx_Semaphore);
	}
}

//========================================================================
//Method: 			Terminal_GPIO_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Prapare Terminal GPIOs for use. 
//operations:		Enables GPIOA(in this platform) CLK
//Params:			-
//Returns:			-
//------------------------------------------------------------------------
static void Terminal_GPIO_Init(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
}

//========================================================================
//Method: 			Terminal_UART_Init
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			Prapare Terminal GPIOs for use.
//operations:		prepares uart instance with 230400 baudrate 
//Notes:			This function should be called from Terminal_Init() 
//					to Initialize uart.
//					TERMINAL_UART_HANDLE.Instance inside the function 
//					should be set according to the Platform used
//Params:			-
//Returns:			-
//------------------------------------------------------------------------
static void Terminal_UART_Init(void) {

	huart2.Instance = USART2;
	huart2.Init.BaudRate = 230400;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;

	/*
	 * Insert the following line in the HAL_UART_Init if already not there.
	 * SET_BIT(huart->Instance->CR1, USART_CR1_IDLEIE);
	 * */
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}
}

//========================================================================
//Method: 			Terminal_RegisterModule
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			To provide a way for other modules to be registered with
//					Terminal
//operations:		If there is still room in pRegisteredModules_hadnles, 
//					stores the handle of the module calling this method in
//					pRegisteredModules_hadnles.
//Params:			
//					tTerminal_ModuleCmdHandle *Modulehandle: handle to be stored
//Returns:			bool: true if registeration was successful
//------------------------------------------------------------------------
bool Terminal_RegisterModule(tTerminal_ModuleCmdHandle *Modulehandle)
{
	if (numberOfRegisteredModules < MAX_TERMINAL_REGISTERED_MODULES)
	{
		pRegisteredModules_hadnles[numberOfRegisteredModules++] = Modulehandle;
		Terminal_Print("Module [%s] registered with Terminal with handle at address: [0x%X]", Modulehandle->name ,Modulehandle);
		return true;
	}
	else
	{
		return false;
		Terminal_Print("Module [%s] could not be registered with Terminal", Modulehandle->name);
	}
}

//========================================================================
//Method: 			Terminal_ModuleCommandsFormatAddToBuffer
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			To Format all the commands and ad it to a buffer
//operations:		
//Params:			
//					char *buffer: buffer to be stored into
//					tTerminal_ModuleCmdHandle *moduleCommandHandle: 
//						Command handle to get the commands. 
//Returns:			-
//------------------------------------------------------------------------
void Terminal_ModuleCommandsFormatAddToBuffer(char *buffer, tTerminal_ModuleCmdHandle *moduleCommandHandle)
{
	char *ptrToTheCmd = moduleCommandHandle->commands;
	memset (buffer,'\0',strlen(buffer));
	while((ptrToTheCmd != NULL) && (*ptrToTheCmd != 0))
	{
		strcat(buffer, "\t\t\t\t");
		strcat(buffer, ptrToTheCmd);
		strcat(buffer, "\n");
		ptrToTheCmd += strlen(ptrToTheCmd) + 1;
	}
}

//========================================================================
//Method: 			Terminal_SearchCommands
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:	
//------------------------------------------------------------------------
//Purpose:			To Search and find a command
//operations:		Replaces end of line with null char
//					While there is a pointer to a command, will continue
//					searching until finds a match.					
//					
//					
//Params:			
//					uint8_t *originalStr: command inside the original string
//					tTerminal_ModuleCmdHandle *moduleCommandHandle: 
//						Command handle to get the commands. 
//Returns:			If found, the Enum Position of the command. otherwise -1 
//------------------------------------------------------------------------
int8_t Terminal_SearchCommands(tTerminal_ModuleCmdHandle *moduleCommandHandle, uint8_t *originalStr)
{
	uint8_t CmdEnumPosition = 0;
	tTerminal_ModuleCmdHandle module = *moduleCommandHandle;

	replace_end_of_line_with_null((char *)originalStr);

	char *ptrToTheCmd = module.commands;
	while((ptrToTheCmd != NULL) && (*ptrToTheCmd != 0))
	{
		if(!strncmp(ptrToTheCmd, (char *)originalStr, strlen(ptrToTheCmd)))
		{
			return CmdEnumPosition;
		}
		CmdEnumPosition++;
		ptrToTheCmd += strlen(ptrToTheCmd) + 1;
	}
	return -1;
}

//========================================================================
//Method: 			Terminal_HandleCommand
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void Terminal_HandleCommand(uint8_t *originalStr)
{
	int8_t found = -1;
	for (uint8_t i = 0; i < numberOfRegisteredModules; i++)
	{
		if(pRegisteredModules_hadnles[i] != NULL)
		{
			found = Terminal_SearchCommands(pRegisteredModules_hadnles[i], originalStr);
			if(found >= 0)
			{
				strtok((char *)originalStr, " ");
				Terminal_Print("Command Found: \'%s\'", originalStr);
				char *pSubCommands = strtok(NULL, " ");
				tTerminal_ModuleCmdHandle moduleToSendCMDTo = *pRegisteredModules_hadnles[i];
				tCmdQueue_message QueueMsg = {.command_enum = found};
				memcpy(QueueMsg.subcommand, pSubCommands, strlen(pSubCommands));
				moduleToSendCMDTo.moduleParseCmdFunction(found, (char *)QueueMsg.subcommand);
				break;
			}
			else
				continue;
		}
		else
		{
			break;
		}
	}
	if(found < 0)
		Terminal_Print("Command not Found: \'%s\'", (char *)originalStr);
}

//========================================================================
//Method: 			Terminal_DumpCommands
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void Terminal_DumpCommands(void)
{
	for (uint8_t i = 0; i < numberOfRegisteredModules; i++)
	{
		if(pRegisteredModules_hadnles[i] != NULL)
		{
			char buffer[100];
			Terminal_ModuleCommandsFormatAddToBuffer(buffer, pRegisteredModules_hadnles[i]);
			Terminal_Print("Module: %s\n%s", pRegisteredModules_hadnles[i]->name, buffer);
		}
		else
			break;
	}

}

//========================================================================
//Method: 			Terminal_ParseCommand
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void Terminal_ParseCommand(uint8_t enumCase, char *SubCommands)
{
	switch (enumCase)
	{
		case HELP:
			Terminal_Print("Dumping registered commands");
			Terminal_DumpCommands();
			break;
		default:
			break;
	}
}

//========================================================================
//Method: 			Terminal_Print
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void Terminal_Print(const char * pFormat, ...) {

	if (Terminal_rx_Semaphore == NULL) return;
	// Semaphore not necessary but doesn't hurt to have in case of concurrent requests.
	if( xSemaphoreTake(Terminal_rx_Semaphore, MS2TICKS(500)) == pdTRUE)
	{
		va_list va;
		va_start(va, pFormat);
		char str[200];
		format_buffer_via_va(str, pFormat, va);
		char taskName[20];
		if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
		{
			strcpy(taskName, "InitTask");
		}
		else
		{
			strncpy(taskName, pcTaskGetName(osThreadGetId()), sizeof(taskName)-1);
		}
		for(int i = strlen(taskName); i < sizeof(taskName); i++)
		{
			if(i == sizeof(taskName) - 1)
			{
				taskName[i] = '\0';
				break;
			}
			taskName[i] = '_';
		}

		sprintf_(printBuffer, "%10.10d.%3.3d| %s| %s\n", RTC_GetRunTime_ms()/1000, RTC_GetRunTime_ms()%1000,taskName, str);
		HAL_UART_Transmit(&TERMINAL_UART_HANDLE, (uint8_t *)&printBuffer, strlen(printBuffer), 10);
		xSemaphoreGive(Terminal_rx_Semaphore);
	}
}

//========================================================================
//Method: 			TerminalThread
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
static void TerminalThread(void const * argument)
{
	Terminal_RegisterModule(&terminalTerminalCommands_Handle);
	if (terminalRxQueueHandle != NULL)
	{
		Terminal_Print("Queue Created: %s", "uartRxQueueHandle");
		HAL_UART_Receive_DMA(&TERMINAL_UART_HANDLE, rxData_buffer, RX_BUFFER_SIZE);
	}
	while (1)
	{
		if (xQueueReceive(terminalRxQueueHandle, &pQueueReceiveMessage, portMAX_DELAY) == pdTRUE)
		{
			Terminal_HandleCommand(pQueueReceiveMessage->message);
		}
	}
}

//========================================================================
//Method: 			HAL_UART_RxHalfCpltCallback
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	Terminal_RxMessageHalfCompletedCallBack(huart);
}

//========================================================================
//Method: 			Terminal_RxMessageHalfCompletedCallBack
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
static void Terminal_RxMessageHalfCompletedCallBack(UART_HandleTypeDef *huart)
{
	static const uint half_way_array_position = RX_BUFFER_SIZE / 2;

	rx_transmission.rx_data_buffer_current_position = half_way_array_position;
	if (rxData_buffer[half_way_array_position -1] != '\n') {
		rx_transmission.data_complete = false;
	} else {
		rx_transmission.data_complete = true;
		rx_transmission.idle_line_should_be_checked = false;
	}
	Terminal_ProcessRxBuffer();
}


//========================================================================
//Method: 			HAL_UART_RxCpltCallback
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
//Purpose:			To Process the received message
//operations:		Checks for Idle line flag therefor the section below
//					should be added to HAL_UART_IRQHandler in STM32 library
//					for idle line detection:
//
//					if (__HAL_UART_GET_FLAG (huart, UART_FLAG_IDLE))
//					{
//					 	HAL_UART_RxCpltCallback (huart);
//					 	__HAL_UART_CLEAR_IDLEFLAG (huart);
//					}				
//					
//Params:			
//					@huart: pointer to uart instance
//Returns:			-
//------------------------------------------------------------------------

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	Terminal_RxMessageCompletedCallBack(huart);
}

//========================================================================
//Method: 			Terminal_RxMessageCompletedCallBack
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
static void Terminal_RxMessageCompletedCallBack(UART_HandleTypeDef *huart)
{
	if (!__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
		//interrupt is due to end of the buffer
		rx_transmission.rx_data_buffer_current_position = RX_BUFFER_SIZE;
		if (rxData_buffer[RX_BUFFER_SIZE - 1] != '\n') {
			rx_transmission.data_complete = false;
		} else {
			rx_transmission.data_complete = true;
			rx_transmission.idle_line_should_be_checked = false;
		}
	} else {
		//interrupt is due to idle line
		//already been taken care of - return
		if (!rx_transmission.idle_line_should_be_checked)
		{
			rx_transmission.idle_line_should_be_checked = true;
			return;
		}
		for (uint16_t i = rx_transmission.rx_data_buffer_last_position;
				i < RX_BUFFER_SIZE; i++) {
			if (rxData_buffer[i] == '\n') {
				rx_transmission.rx_data_buffer_current_position = ++i;
				rx_transmission.data_complete = true;
				break;
			}
		}
	}

	Terminal_ProcessRxBuffer();
}

//========================================================================
//Method: 			Terminal_ProcessRxBuffer
//Last Updated: 	13-03-2020
//Modifier:			Sahand M
//Rectified Bugs:
//------------------------------------------------------------------------
//Purpose:			to process data received on uart
//operations:				
//Params:			-
//Returns:			-
//------------------------------------------------------------------------
/*@brief this function processes the received data on uart*/
static void Terminal_ProcessRxBuffer(void) {

	uint16_t copy_size = rx_transmission.rx_data_buffer_current_position - rx_transmission.rx_data_buffer_last_position;

	if (rx_transmission.cached_data_buffer_current_position + copy_size > RX_CACHED_DATA_SIZE - 1) return;

	memcpy(&rxData[rx_transmission.cached_data_buffer_current_position],
			&rxData_buffer[rx_transmission.rx_data_buffer_last_position],
			copy_size);

	rx_transmission.rx_data_buffer_last_position =
			rx_transmission.rx_data_buffer_current_position;

	if (rx_transmission.rx_data_buffer_last_position == RX_BUFFER_SIZE) {
		rx_transmission.rx_data_buffer_last_position = 0;
	}

	rx_transmission.cached_data_buffer_current_position += copy_size;

	if (rx_transmission.data_complete == true) {

		rx_transmission.rx_data_size = rx_transmission.cached_data_buffer_current_position;
		rx_transmission.cached_data_buffer_current_position = 0;
		rx_transmission.data_complete = false;
		rxData[rx_transmission.rx_data_size] = '\0';
		pQueueSendMessage = (tUART_message *)&rxData;
		BaseType_t higherPrioritytaskWoken;
		xQueueSendFromISR(terminalRxQueueHandle, (void *)&pQueueSendMessage, &higherPrioritytaskWoken);
		portYIELD_FROM_ISR(higherPrioritytaskWoken);
	}
	HAL_UART_Receive_DMA(&TERMINAL_UART_HANDLE, rxData_buffer, RX_BUFFER_SIZE);
}

#endif
