/**
 * @file main.c
 * @author Ahmed_Wagdy_Mohy
 * @brief  the main code goes here - CLEAN CODE Rules is applied :))
 * @version 0.1
 * @date 2024-06-14
 *
 * @copyright  (c) 2024 ,Ahmed_Wagdy All Rights Reserved
 *
 */










//************************* <includes section *****************************************************************

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include <stdio.h>
#include <stdlib.h>

#define CCM_RAM __attribute__((section(".ccmram")))

//************************* </includes *****************************************************************












//*********************<  defines *******************************************************
#define QUEUE_SIZE 10
#define INITIAL_T_SENDER_LOWER 50
#define INITIAL_T_SENDER_UPPER 150
#define T_RECEIVER 100
//*********************< /defines *******************************************************









//********************< Function prototypes ***************************************************************

void senderTask(void *params);
void receiverTask(void *params);
void senderTimerCallback(TimerHandle_t xTimer);
void receiverTimerCallback(TimerHandle_t xTimer);
void resetSystem();

//********************< Function prototypes **************************************************












//**********************< Global variables**********************************************************


QueueHandle_t xQueue;
/* 3 Semaphores for each sender task */
SemaphoreHandle_t xSenderSemaphore1, xSenderSemaphore2, xSenderSemaphoreHigh;

/* 1 Semaphore for receiving task*/
SemaphoreHandle_t xReceiverSemaphore;

/* 3 Timers for each sender task */
TimerHandle_t xSenderTimer1, xSenderTimer2, xSenderTimerHigh;

/* 1 Timer for receiving task*/
TimerHandle_t xReceiverTimer;



/* Intialized Global Variables used */
uint32_t sentMessages1 = 0, blockedMessages1 = 0;
uint32_t sentMessages2 = 0, blockedMessages2 = 0;
uint32_t sentMessagesHigh = 0, blockedMessagesHigh = 0;
uint32_t receivedMessages = 0;


uint32_t tSenderLowerBounds[] = {50, 80, 110, 140, 170, 200};
uint32_t tSenderUpperBounds[] = {150, 200, 250, 300, 350, 400};
size_t currentBoundIndex = 0;


//**********************< Global variables*****************************************************************/
















//*********************< Leds defines (not used) *******************************************************


//*********************< /Leds defines (not used) *******************************************************












//***********************************< Pragmas*************************************************************
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
//***********************************< /Pragmas*****************************************************************















//**************************************** <Main() *************************************************************
//**********************************************************************(c)Ahmed_Wagdy All Rights Reserved******
void main(void)
{

	/* Creating the Queue will be used for the communications*/
	xQueue = xQueueCreate(QUEUE_SIZE , sizeof(char[20]));


	/* Creating Semaphores "Binary Sem will be used no use for the Counting here" */
	xSenderSemaphore1 =     	xSemaphoreCreateBinary();
	xSenderSemaphore2 =    		xSemaphoreCreateBinary();
	xSenderSemaphoreHigh= 		xSemaphoreCreateBinary();
	xReceiverSemaphore =    	xSemaphoreCreateBinary();





	/*Creating the Tasks will be Used*/

	/*NOTES : for the StackDepth I have used the Minimal option from the Official RTOS doc  AND pxCreatedTask is assigned to NULL*/
	/*NOTES:  for the priority I have Used the IdleTask priority as a reference and then added to it the priorities */
	/*NOTES:  (void *)1  it's the integer 1 cast to a void to pass an Id for each task to distinguish them */
	xTaskCreate(senderTask, "SenderTask1", configMINIMAL_STACK_SIZE, (void *)1 , tskIDLE_PRIORITY + 1 , NULL);
	xTaskCreate(senderTask ,"SenderTask2", configMINIMAL_STACK_SIZE, (void *)2 , tskIDLE_PRIORITY + 1 , NULL );
	xTaskCreate(senderTask, "SenderHigh", configMINIMAL_STACK_SIZE, (void *)3, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(receiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);






	/*Creating the Timers will be Used*/

	/*NOTES:  for the xTimerPeriod and uxAutoReload I have Used the Defaults of the RTOS Reference!!!!!! PdTRUE for autoreload Timer */
	xSenderTimer1    = 	xTimerCreate("SenderTimer1", pdMS_TO_TICKS(INITIAL_T_SENDER_UPPER) ,pdTRUE , (void *)1 , senderTimerCallback );
	xSenderTimer2    =  xTimerCreate("SenderTimer2", pdMS_TO_TICKS(INITIAL_T_SENDER_UPPER), pdTRUE, (void *)2, senderTimerCallback);
	xSenderTimerHigh = 	xTimerCreate("SenderTimerHigh", pdMS_TO_TICKS(INITIAL_T_SENDER_UPPER), pdTRUE, (void *)3, senderTimerCallback);
	xReceiverTimer   = 	xTimerCreate("ReceiverTimer", pdMS_TO_TICKS(T_RECEIVER), pdTRUE, NULL, receiverTimerCallback);


	xTimerStart(xSenderTimer1 , 0);
	xTimerStart(xSenderTimer2, 0);
	xTimerStart(xSenderTimerHigh, 0);
	xTimerStart(xReceiverTimer, 0);




	vTaskStartScheduler();


	for (;;);




}

#pragma GCC diagnostic pop

//**************************************** </Main() *************************************************************
//*************************************************************************************************************




















//**************************************** <Functions Implementations *************************************************************

void senderTask(void *params){
	/*Converting the ID passed to and an integer*/
	int taskId  = (int)params;

	SemaphoreHandle_t xSenderSemaphore;
	/* Pointers for the messages -> will be assigned based on the task ID*/
	uint32_t *SentMessages ,*BlockedMessages;





	/* If cond on the Task ID*/
	if(taskId  == 1){

		xSenderSemaphore = xSenderSemaphore1;
		SentMessages = &sentMessages1;
		BlockedMessages =&blockedMessages1;

	}//if
	else if(taskId == 2){


		xSenderSemaphore = xSenderSemaphore2;
		SentMessages = &sentMessages2;
		BlockedMessages =&blockedMessages2;

	}//else if
	else{

		xSenderSemaphore = xSenderSemaphoreHigh;
		SentMessages = &sentMessagesHigh;
		BlockedMessages =&blockedMessagesHigh;

	}//else




	char message[20];
	/*NOTE : total number of tick interrupts that have occurred since the scheduler was started*/
	TickType_t xLastWakeTime = xTaskGetTickCount();




/*************************** SUPER LOOP OF THE TASK *******************************************/


	for(;;){

		/*NOTE: xTicksToWait to portMAX_DELAY will cause the task to wait indefinitely*/
		xSemaphoreTake(xSenderSemaphore ,portMAX_DELAY);
		/*Creating a message with the current time */
		snprintf(message, sizeof(message), "Time is %lu", xTaskGetTickCount());

		if(xQueueSend(xQueue , &message, 0 ) == pdPASS){
			(*SentMessages)++;
		}//Queueif
		else{
			(*BlockedMessages)++;
		}//QueueElse




		//*Delay to sleep a random amount of time*//
		vTaskDelay(pdMS_TO_TICKS(rand() % (tSenderUpperBounds[currentBoundIndex] - tSenderLowerBounds[currentBoundIndex]) + tSenderLowerBounds[currentBoundIndex]));



	}//SuperLoop



}//senderTask









void receiverTask(void *params){
	char message[20];
	TickType_t xLastWakeTime = xTaskGetTickCount();


/*************************** SUPER LOOP OF THE TASK *******************************************/
	for(;;){

		xSemaphoreTake(xReceiverSemaphore, portMAX_DELAY);


		if(xQueueReceive(xQueue, &message , 0) == pdPASS){

			receivedMessages++;
			printf("Received: %s\n", message);
		}

		/*Condition of Reseting the System */
		if(receivedMessages>=1000){
			resetSystem();
		}


	}//superloop


}//receiverTask








void senderTimerCallback(TimerHandle_t xTimer){
	/*Getting the Timer ID from the Arguments and convreting it to integer!*/
	int taskId = (int)pvTimerGetTimerID(xTimer);

	//Releasing the semaphore for each sender task
	if(taskId  == 1){
		xSemaphoreGive(xSenderSemaphore1);

	}//if
	else if(taskId == 2){
		xSemaphoreGive(xSenderSemaphore2);

	}//else if
	else{
		xSemaphoreGive(xSenderSemaphoreHigh);

	}//else
}












void receiverTimerCallback(TimerHandle_t xTimer) {
    xSemaphoreGive(xReceiverSemaphore);

    if (receivedMessages >= 1000) {
        resetSystem();
    }
}





















void resetSystem() {
    // Print statistics
    printf("Statistics:\n");
    printf("Sender1 - Sent: %lu, Blocked: %lu\n", sentMessages1, blockedMessages1);
    printf("Sender2 - Sent: %lu, Blocked: %lu\n", sentMessages2, blockedMessages2);
    printf("SenderHigh - Sent: %lu, Blocked: %lu\n", sentMessagesHigh, blockedMessagesHigh);
    printf("Received: %lu\n", receivedMessages);

    // Reset counters
    sentMessages1 = 0;
    blockedMessages1 = 0;
    sentMessages2 = 0;
    blockedMessages2 = 0;
    sentMessagesHigh = 0;
    blockedMessagesHigh = 0;
    receivedMessages = 0;

    // Clear queue
    xQueueReset(xQueue);

    // Update timer periods
    currentBoundIndex++;
    if (currentBoundIndex >= sizeof(tSenderLowerBounds) / sizeof(tSenderLowerBounds[0])) {
        // Game over
        printf("Game Over\n");
        xTimerStop(xSenderTimer1, 0);
        xTimerStop(xSenderTimer2, 0);
        xTimerStop(xSenderTimerHigh, 0);
        xTimerStop(xReceiverTimer, 0);
        vTaskEndScheduler();
    } else {
        xTimerChangePeriod(xSenderTimer1, pdMS_TO_TICKS(tSenderUpperBounds[currentBoundIndex]), 0);
        xTimerChangePeriod(xSenderTimer2, pdMS_TO_TICKS(tSenderUpperBounds[currentBoundIndex]), 0);
        xTimerChangePeriod(xSenderTimerHigh, pdMS_TO_TICKS(tSenderUpperBounds[currentBoundIndex]), 0);
    }
}











//**************************************** </Functions Implementations *************************************************************










void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
