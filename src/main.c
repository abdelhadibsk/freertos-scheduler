#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"
#include <stdio.h>

/* =========================================================
 * Task handles
 * ========================================================= */
TaskHandle_t tA, tB, tC;

/* =========================================================
 * Forward declarations
 * ========================================================= */
void periodic_task( void *pvParameters );
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );
void MyTaskSwitchedIn( void );
void MyTaskSwitchedOut( void );

/* =========================================================
 * main
 * ========================================================= */
int main( void )
{
    printf( "MAIN START\n" );
    setvbuf( stdout, NULL, _IONBF, 0 );

    /* Create RT periodic tasks.
     * No need to pass parameters to the task function —
     * everything is stored inside the TCB via xRTTaskCreate. */
    xRTTaskCreate( periodic_task, "A", 1024, NULL,
                   pdMS_TO_TICKS( 100 ),   /* period   */
                   pdMS_TO_TICKS( 100 ),   /* deadline */
                   pdMS_TO_TICKS( 20  ),   /* wcet     */
                   &tA );

    xRTTaskCreate( periodic_task, "B", 1024, NULL,
                   pdMS_TO_TICKS( 200 ),
                   pdMS_TO_TICKS( 200 ),
                   pdMS_TO_TICKS( 40  ),
                   &tB );

    xRTTaskCreate( periodic_task, "C", 1024, NULL,
                   pdMS_TO_TICKS( 400 ),
                   pdMS_TO_TICKS( 400 ),
                   pdMS_TO_TICKS( 60  ),
                   &tC );

    /* Print registered task info using helper API */
    printf( "Tasks created and registered:\n" );
    for( UBaseType_t i = 0; i < uxRTGetTaskCount(); i++ )
    {
        TaskHandle_t h = xRTGetTaskByIndex( i );
        printf( "  Task %s: period=%lu  deadline=%lu  wcet=%lu\n",
                pcTaskGetName( h ),
                ( unsigned long ) xRTGetTaskPeriod( h ),
                ( unsigned long ) xRTGetTaskDeadline( h ),
                ( unsigned long ) xRTGetTaskWCET( h ) );
    }

    /* Active policy shown at startup */
#if   ( configUSE_RM   == 1 )
    printf( "Policy: Rate Monotonic (RM)\n" );
#elif ( configUSE_DM   == 1 )
    printf( "Policy: Deadline Monotonic (DM)\n" );
#elif ( configUSE_FIFO == 1 )
    printf( "Policy: FIFO\n" );
#elif ( configUSE_EDF  == 1 )
    printf( "Policy: Earliest Deadline First (EDF)\n" );
#endif

    fflush( stdout );

    printf( "Starting scheduler\n" );
    vTaskStartScheduler();

    for( ;; );
    return 0;
}

/* =========================================================
 * PERIODIC TASK
 *
 * No parameters passed — everything is read directly from
 * the TCB via the helper API using the current task handle.
 *
 * Pattern:
 *   1. Do work (busy wait for wcet duration)
 *   2. Print job info
 *   3. Suspend — TickHook will release next job
 * ========================================================= */
void periodic_task( void *pvParameters )
{
    ( void ) pvParameters;

    for( ;; )
    {
        TaskHandle_t self    = xTaskGetCurrentTaskHandle();
        TickType_t   wcet    = xRTGetTaskWCET( self );
        TickType_t   start   = xTaskGetTickCount();

        printf( "[START] %s  tick=%lu  wcet=%lu\n",
                pcTaskGetName( self ),
                ( unsigned long ) start,
                ( unsigned long ) wcet );

        /* Simulate workload — busy wait for wcet duration */
        while( ( xTaskGetTickCount() - start ) < wcet )
        {
            /* intentionally empty */
        }

        TickType_t finish = xTaskGetTickCount();
        printf( "[END  ] %s  tick=%lu  exec=%lu\n",
                pcTaskGetName( self ),
                ( unsigned long ) finish,
                ( unsigned long ) ( finish - start ) );

        /* Job done — TickHook will resume at next release */
        vTaskSuspend( NULL );
    }
}

/* =========================================================
 * Hook — Stack Overflow
 * ========================================================= */
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    ( void ) xTask;
    printf( "STACK OVERFLOW: %s\n", pcTaskName );
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

/* =========================================================
 * Hooks — Task Switch Trace
 * Called by FreeRTOS trace macros (traceTASK_SWITCHED_IN /
 * traceTASK_SWITCHED_OUT) — define these macros in
 * FreeRTOSConfig.h to enable tracing:
 *
 *   #define traceTASK_SWITCHED_IN()  MyTaskSwitchedIn()
 *   #define traceTASK_SWITCHED_OUT() MyTaskSwitchedOut()
 * ========================================================= */
void MyTaskSwitchedIn( void )
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();
    printf( "[IN ] %s at %lu\n",
            pcTaskGetName( h ),
            ( unsigned long ) xTaskGetTickCount() );
}

void MyTaskSwitchedOut( void )
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();
    printf( "[OUT] %s at %lu\n",
            pcTaskGetName( h ),
            ( unsigned long ) xTaskGetTickCount() );
}

/* =========================================================
 * Hook — Tick
 * vApplicationRTTickHook() is called automatically inside
 * xTaskIncrementTick() via the tasks.c modification.
 * This hook is for application-level periodic logging only.
 * ========================================================= */
void vApplicationTickHook( void )
{
    static TickType_t tickCount = 0;
    tickCount++;

    if( ( tickCount % 100 ) == 0 )
    {
        printf( "[TICK] %lu\n", ( unsigned long ) tickCount );
    }
}