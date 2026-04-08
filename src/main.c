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
void vApplicationTickHook( void );
void show_specific_task_states(void);

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
                   pdMS_TO_TICKS( 200 ),   /* period   */
                   pdMS_TO_TICKS( 200 ),   /* deadline */
                   pdMS_TO_TICKS( 80  ),   /* execution time */
                   &tA );

    xRTTaskCreate( periodic_task, "B", 1024, NULL,
                   pdMS_TO_TICKS( 300 ),
                   pdMS_TO_TICKS( 300 ),
                   pdMS_TO_TICKS( 80  ),
                   &tB );

    xRTTaskCreate( periodic_task, "C", 1024, NULL,
                   pdMS_TO_TICKS( 1000 ),
                   pdMS_TO_TICKS( 1000 ),
                   pdMS_TO_TICKS( 600  ),
                   &tC );

    /* Print registered task info using helper API */
    printf( "Tasks created and registered:\n" );
    for( UBaseType_t i = 0; i < uxRTGetTaskCount(); i++ )
    {
        TaskHandle_t h = xRTGetTaskByIndex( i );
        printf( "  Task %s: period=%lu  deadline=%lu  execution_time=%lu\n",
                pcTaskGetName( h ),
                ( unsigned long ) xRTGetTaskPeriod( h ),
                ( unsigned long ) xRTGetTaskDeadline( h ),
                ( unsigned long ) xRTGetTaskExecutionTime( h ) );
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
 *   1. Do work (busy wait for execution time duration)
 *   2. Print job info
 *   3. Suspend — TickHook will release next job
 * ========================================================= */
void periodic_task( void *pvParameters )
{
    ( void ) pvParameters;

    for( ;; )
    {
        TaskHandle_t self    = xTaskGetCurrentTaskHandle();
        TickType_t   execution_time = xRTGetTaskExecutionTime( self );
        TickType_t   elapsed = 0;
        TickType_t   slice_start;

        /*printf( "[START] %s  tick=%lu  execution_time = %lu\n",
                pcTaskGetName( self ),
                ( unsigned long ) xTaskGetTickCount(),
                ( unsigned long ) execution_time );
        */

        /* Exécute par tranches — accumule seulement le temps CPU réel.
         * Si préemptée entre deux tranches, le temps perdu n'est pas compté. */
        while( elapsed < execution_time )
        {
            slice_start = xTaskGetTickCount();

            /* Tranche de travail — 1 tick à la fois */
            while( xTaskGetTickCount()< (slice_start + 1) )
            {
                /* busy work pendant exactement 1 tick */
            }

            elapsed++;   /* 1 tick CPU consommé */
        }

        /*TickType_t finish = xTaskGetTickCount();
        printf( "[END  ] %s  tick=%lu  exec=%lu\n",
                pcTaskGetName( self ),
                ( unsigned long ) finish,
                ( unsigned long ) elapsed );
        */
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
/*
void vApplicationTickHook( void )
{
    static TickType_t tickCount = 0;
    tickCount++;

    if( ( tickCount % 100 ) == 0 )
    {
        printf( "[TICK] %lu\n", ( unsigned long ) tickCount );
    }
}
*/

void show_specific_task_states(void)
{
    TaskHandle_t tasks[] = {tA, tB, tC};
    const char* names[] = {"A", "B", "C"};
    
    printf("\n--- Specific Task States ---\n");
    for(int i = 0; i < 3; i++)
    {
        if(tasks[i] != NULL)
        {
            eTaskState state = eTaskGetState(tasks[i]);
            const char *state_str;
            
            switch(state)
            {
                case eRunning:   state_str = "Running"; break;
                case eReady:     state_str = "Ready"; break;
                case eBlocked:   state_str = "Blocked"; break;
                case eSuspended: state_str = "Suspended"; break;
                case eDeleted:   state_str = "Deleted"; break;
                default:         state_str = "Unknown"; break;
            }
            
            printf("Task %s: %s\n", names[i], state_str);
        }
    }
    printf("----------------------------\n");
}

// And update the tick hook to use the simpler version:
void vApplicationTickHook(void)
{
    static TickType_t tickCount = 0;
    tickCount++;
    
    if ((tickCount % 10) == 0)
    {
        printf("[TICK] %lu ticks elapsed\n", (unsigned long)tickCount);
        
        if ((tickCount % 50) == 0) 
        {
            show_specific_task_states();
        }
    }
}