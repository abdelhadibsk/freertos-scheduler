/* =========================================================
 * scheduler.c
 * RT Scheduler Extension — Core Engine
 *
 * This file contains ONLY the 3 hooks and the task wrapper.
 * It never touches TCB_t directly.
 * All TCB access goes through the helpers defined in tasks.c.
 * ========================================================= */

#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

/* Policy functions — defined in policies.c */
extern void vRM_UpdatePriorities( void );
extern void vDM_UpdatePriorities( void );
extern void vFIFO_UpdatePriorities( void );
extern void vEDF_UpdatePriorities( void );

/* =========================================================
 * xRTTaskCreate — Task Creation Wrapper
 * ========================================================= */
BaseType_t xRTTaskCreate(
    TaskFunction_t         pxTaskCode,
    const char * const     pcName,
    configSTACK_DEPTH_TYPE uxStackDepth,
    void *                 pvParameters,
    TickType_t             period,
    TickType_t             deadline,
    TickType_t             execution_time,
    TaskHandle_t *         pxCreatedTask )
{
    BaseType_t xResult;

    xResult = xTaskCreate(
                  pxTaskCode,
                  pcName,
                  uxStackDepth,
                  pvParameters,
                  tskIDLE_PRIORITY + 1U,
                  pxCreatedTask );

    /* if the task is created xResult = pdPASS 
     * else if the creation fails xResult = pdFALSE */

    if( xResult == pdPASS ) 
    {
        /* Register RT params — implemented in tasks.c, has TCB access */
        vApplicationRTTaskRegister( *pxCreatedTask, period, deadline, execution_time );

        

        /* Suspend — TickHook controls first release */
        vTaskSuspend( *pxCreatedTask );
    }

    return xResult;
}

/* =========================================================
 * Hook 2 — vApplicationSchedulerTickHook
 * Called every tick by xTaskIncrementTick() in tasks.c.
 * ISR context — only FromISR APIs allowed. because of the critical section inside xTaskIncrementTick(), we are guaranteed that only one tick hook runs at a time.
 * this hook is responsible for releasing jobs at their next_release time and resuming tasks if needed.
 * release logic:
 *   for each task, check if xNow >= next_release (read via helper). If so, call vRTJobRelease (helper) to update RT fields for the new job, and resume the task if it was suspended.
 * ========================================================= */
void vApplicationSchedulerTickHook( void )
{
    UBaseType_t i;
    BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
    TickType_t  xNow = xTaskGetTickCountFromISR();

    UBaseType_t n = uxRTGetTaskCount();   /* helper in tasks.c */

    /* Loop through all registered tasks and release those whose next_release has arrived.
     * Note: we could optimize this by keeping a sorted list of next_release times, 
     * but for simplicity we just loop through all tasks every tick. */
    for( i = 0; i < n; i++ )
    {
        TaskHandle_t xTask = xRTGetTaskByIndex( i );   /* helper in tasks.c */

        /* Read next_release — we need a local ISR-safe read.
         * Since we are in ISR context we use a direct field access
         * via a dedicated helper (no critical section needed inside
         * ISR as tick interrupt is the only writer of next_release). */
        TickType_t next_release = xRTGetNextRelease( xTask );

        if( xNow >= next_release )
        {
            /* New job — update RT fields via ISR-safe helper */
            vRTJobRelease( xTask, xNow );   /* helper in tasks.c */
            
            if( eTaskGetState( xTask ) == eSuspended )  
            {
                xTaskResumeFromISR( xTask );
                xHigherPriorityTaskWoken = pdTRUE;
            }
                
        }
        #if (configUSE_EDF == 1)
        /* For EDF, we need to update absolute_deadline on every release, even if the task is not ready, to ensure correct priority ranking. 
         * This is handled inside vRTJobRelease, which updates absolute_deadline based on the new job's parameters. */
        
        // we should add a function that updates absolute_deadline for all tasks on every tick,
         
        
        #endif
    }

    if( xHigherPriorityTaskWoken == pdTRUE )
    {
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

/* =========================================================
 * Hook 3 — vApplicationSchedulerUpdatePriorities
 * Called by vTaskSwitchContext() before highest priority selection.
 * Already inside a critical section — no extra protection needed.
 * ========================================================= */
void vApplicationSchedulerUpdatePriorities( void )
{
#if   ( configUSE_RM   == 1 )
    vRM_UpdatePriorities();
#elif ( configUSE_DM   == 1 )
    vDM_UpdatePriorities();
#elif ( configUSE_FIFO == 1 )
    vFIFO_UpdatePriorities();
#elif ( configUSE_EDF  == 1 )
    vEDF_UpdatePriorities();
#endif
}