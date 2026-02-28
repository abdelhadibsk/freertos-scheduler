/* =========================================================
 * rt_scheduler.c
 * RT Scheduler Extension — Core Engine
 *
 * This file contains ONLY the 3 hooks and the task wrapper.
 * It never touches TCB_t directly.
 * All TCB access goes through the helpers defined in tasks.c.
 * ========================================================= */

#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

/* Policy functions — defined in rt_policies.c */
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
    TickType_t             wcet,
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

    if( xResult == pdPASS )
    {
        /* Register RT params — implemented in tasks.c, has TCB access */
        vApplicationRTTaskRegister( *pxCreatedTask, period, deadline, wcet );

        /* Suspend — TickHook controls first release */
        vTaskSuspend( *pxCreatedTask );
    }

    return xResult;
}

/* =========================================================
 * Hook 2 — vApplicationRTTickHook
 * Called every tick by xTaskIncrementTick() in tasks.c.
 * ISR context — only FromISR APIs allowed.
 * ========================================================= */
void vApplicationSchedulerTickHook( void )
{
    UBaseType_t i;
    BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
    TickType_t  xNow = xTaskGetTickCountFromISR();

    UBaseType_t n = uxRTGetTaskCount();   /* helper in tasks.c */

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
            /*
            if( eTaskGetState( xTask ) == eSuspended )  
            {
                vTaskResumeFromISR( xTask );
                xHigherPriorityTaskWoken = pdTRUE;
            }
                */
        }
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