/*
 * UI.c
 *
 *  Created on: 8Mar.,2020
 *      Author: Sahand Maleki
 */

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// 				_ _  _  ___ _    _  _  __  ____  ___
// 				| |\ | |    |    |  | |  \ |___ [__
// 				| | \| |___ |___ |__| |__/ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "UI.h"
#include "timers.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//			  __  ____ ____ _ _  _ _ ___ _  __  _  _  ___
//			 |  \ |___ |___ | |\ | |  |  | |  | |\ | [__
//			 |__/ |___ |    | | \| |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define UNDEBOUNCED_VALUE 	0x00
#define DEBOUNCED_VALUE   	0xFF
#define DEBOUNCE_PERIOD_MS 	5

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  __   __  _ _  _ __  ___ ____    _  _ ____ ___ _  _  __   __   ___
// |__] |__/ | |  ||__|  |  |___    |\/| |___  |  |__| |  | |  \ [__
// |    |  \ |  \/ |  |  |  |___    |  | |___  |  |  | |__| |__/ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void UI_Thread(void const * argument);
void debounceTimerCallback(TimerHandle_t xTimer);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//				_  _ __   __  _ __   __  _    ____  ___
//				|  ||__| |__/ ||__| |__] |    |___ [__
//				 \/ |  | |  \ ||  | |__] |___ |___ ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static bool 						gShouldSample = false;
extern osThreadId 					IMUTaskHandle;
static uint16_t 					blinking_period = 250;
static uint8_t 						debounceFIFO = UNDEBOUNCED_VALUE;
static TimerHandle_t 				debounceTimer;
static tTerminal_ModuleCmdHandle 	uiTerminalCommands_Handle = {
											.commands = {"led-blinking-fast\0led-blinking-slow\0led-blinking-period\0button-pressed"},
											.name = UI_MODULE_NAME,
											.moduleParseCmdFunction = UI_parse_cmd};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// _  _ ____ ___ _  _  __   __     _ _  _  __  _    ____ _  _ ____ _  _ ___ __  ___ _  __  _  _  ___
// |\/| |___  |  |__| |  | |  \    | |\/| |__] |    |___ |\/| |___ |\ |  | |__|  |  | |  | |\ | [__
// |  | |___  |  |  | |__| |__/    | |  | |    |___ |___ |  | |___ | \|  | |  |  |  | |__| | \| ___]
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static void UI_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PushButton_Pin */
  GPIO_InitStruct.Pin = PushButton_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PushButton_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);
}

void UI_Init(void)
{
	UI_GPIO_Init();

	debounceTimer = xTimerCreate
	                   ( /* Just a text name, not used by the RTOS
	                     kernel. */
	                     "debounceTimer",
	                     /* The timer period in ticks, must be
	                     greater than 0. */
	                     MS2TICKS(DEBOUNCE_PERIOD_MS),
	                     /* The timers will auto-reload themselves
	                     when they expire. */
	                     pdTRUE,
	                     /* The ID is used to store a count of the
	                     number of times the timer has expired, which
	                     is initialised to 0. */
	                     ( void * ) 0,
	                     /* Each timer calls the same callback when
	                     it expires. */
	                     debounceTimerCallback
	                   );

    if( debounceTimer == NULL )
    {
    	Terminal_Print("Could not create debouncing Timer.");
        return;
    }
    else
    {
        /* Start the timer.  No block time is specified, and
        even if one was it would be ignored because the RTOS
        scheduler has not yet been started. */

    }

	osThreadDef(uiTask, UI_Thread, osPriorityNormal, 0, 1024);
	uiTaskHandle = osThreadCreate(osThread(uiTask), NULL);

	Terminal_RegisterModule(&uiTerminalCommands_Handle);

}

void UI_parse_cmd(uint8_t enumCase, char *subcommand)
{
	uint32_t period;
	switch (enumCase)
	{
	case LED_BLINKING_FAST:
		blinking_period = 100;
		Terminal_Print("Setting blinking period to: %d", blinking_period);
		break;
	case LED_BLINKING_SLOW:
		blinking_period = 250;
		Terminal_Print("Setting blinking period to: %d", blinking_period);
		break;
	case LED_BLINKING_PERIOD:
		period = atoi(subcommand);
		if(period == 0)
		{
			Terminal_Print("SubCommand '%s' Must be a number and greater than zero", subcommand);
		}
		else
		{
			Terminal_Print("Setting blinking period to: %d", period);
			blinking_period = period;
		}
		break;
	case BUTTON_PRESSED:
		gShouldSample = !gShouldSample;
		uint32_t ulValue;
		if(gShouldSample)
		{
			ulValue = START_SAMPLING;
		}
		else
		{
			ulValue = STOP_SAMPLING;
		}
		xTaskNotify(IMUTaskHandle, ulValue ,eSetValueWithOverwrite);
		break;
	default:
		break;
	}
}


void UI_Thread(void const * argument)
{

  for(;;)
  {
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    osDelay(blinking_period);
  }
  /* USER CODE END 5 */
}

static volatile uint16_t timerStartCount = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if( debounceTimer == NULL && (xTimerIsTimerActive(debounceTimer) != pdFALSE))
    {
        return;
    }
    else
    {
        /* Start the timer.  No block time is specified, and
        even if one was it would be ignored because the RTOS
        scheduler has not yet been started. */
    	if( xTimerStartFromISR( debounceTimer, &xHigherPriorityTaskWoken ) != pdPASS )
		{
			/* The start command was not executed successfully.  Take appropriate
			action here. */
		}
    	timerStartCount++;
    }
}

void debounceTimerCallback(TimerHandle_t xTimer)
{

	debounceFIFO = (debounceFIFO << 1) | HAL_GPIO_ReadPin(PushButton_GPIO_Port, PushButton_Pin);
	if (debounceFIFO == DEBOUNCED_VALUE)
	{
		debounceFIFO = UNDEBOUNCED_VALUE;

		if( xTimerStop( xTimer , 0 ) != pdPASS )
		{
			debounceFIFO = UNDEBOUNCED_VALUE;
			return;
			/* The stop command was not executed successfully.  Take appropriate
			action here. */
		}

		gShouldSample = !gShouldSample;
		uint32_t ulValue;
		if(gShouldSample)
		{
			ulValue = START_SAMPLING;
		}
		else
		{
			ulValue = STOP_SAMPLING;
		}
		xTaskNotify(IMUTaskHandle, ulValue ,eSetValueWithOverwrite);
	}

}
