/*
 * terminal.h
 *
 *  Created on: 3Jan.,2020
 *      Author: Sahand Maleki
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "projectOptions.h"
#if PROJECT_TERMINAL_ENABLED
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "Printf/printf.h"
#include "Utility/utility.h"
#include <stdbool.h>


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define TERMINAL_TASK_STACK_SIZE 2048
#define MAX_TERMINAL_REGISTERED_MODULES 5

#define RX_BUFFER_SIZE 	30 // This should be an even number
#define RX_CACHED_DATA_SIZE RX_BUFFER_SIZE*6

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 		 __  _  _  __  _    _  ___    ___ _   _  __  ____  ___
// 		|__] |  | |__] |    | |        |   \_/  |__] |___ [__
// 		|    |__| |__] |___ | |___     |    |   |    |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
typedef struct{
	uint16_t 	rx_data_buffer_last_position;
	uint16_t 	rx_data_buffer_current_position;
	uint16_t 	cached_data_buffer_current_position;
	uint16_t	rx_data_size;
	bool	 	data_complete;
	bool 		idle_line_should_be_checked;
}tURAT_RX_transmission;

typedef struct{
	char			name[10];
	char			commands[200];
	void 			(*moduleParseCmdFunction)(uint8_t enumCase, char *pSubCommands);

}tTerminal_ModuleCmdHandle;

typedef struct{
	uint8_t 	message[RX_CACHED_DATA_SIZE];
	uint16_t	size;
}tUART_message;

typedef struct{
	uint8_t 	message[RX_CACHED_DATA_SIZE];
}tTerminalCmd_message;

typedef struct{
	uint8_t 	subcommand[30];
	uint8_t 	command_enum;
}tCmdQueue_message;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 	 __  _  _  __  _    _  ___    _  _ ____ ___ _  _  __   __   ___
// 	|__] |  | |__] |    | |       |\/| |___  |  |__| |  | |  \ [__
// 	|    |__| |__] |___ | |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void 	Terminal_Init(void);
bool	Terminal_RegisterModule(tTerminal_ModuleCmdHandle *Modulehandle);
void 	Terminal_Print(const char * pFormat, ...);
void 	Terminal_HandleCommand(uint8_t *originalStr);
void 	Terminal_ParseCommand(uint8_t enumCase, char *pSubCommands);

osThreadId terminalTaskHandle;
UART_HandleTypeDef huart2;
#define TERMINAL_UART_HANDLE huart2




DMA_HandleTypeDef hdma_usart2_rx;
QueueHandle_t 		terminalRxQueueHandle;







#endif


#endif /* TERMINAL_H_ */
