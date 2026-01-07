#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION            0
#define configRUN_MULTIPLE_PRIORITIES    1

#define configUSE_TIME_SLICING          1
#define configCPU_CLOCK_HZ              ( ( unsigned long ) 100000000 )
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 )

#define configMAX_PRIORITIES            10
#define configMINIMAL_STACK_SIZE        256
#define configTOTAL_HEAP_SIZE           ( 20 * 1024 )

#define configUSE_TASK_NOTIFICATIONS    1

#define INCLUDE_vTaskDelay      1
#define INCLUDE_vTaskSuspend    1
#define INCLUDE_vTaskResume     1

#define INCLUDE_uxTaskPriorityGet  1

#define configCHECK_FOR_STACK_OVERFLOW  2

#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             1

/* Enable task state hook */
#define configUSE_TASK_STATE_HOOK       1

#define configUSE_TRACE_FACILITY        1
#define configUSE_16_BIT_TICKS          0

#define INCLUDE_vTaskDelayUntil    1

/* Keep stats formatting but remove run-time stats */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
/* Remove this line: #define configGENERATE_RUN_TIME_STATS   1 */

#define configASSERT(x) \
    if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

#define traceTASK_SWITCHED_IN()   MyTaskSwitchedIn()
#define traceTASK_SWITCHED_OUT()  MyTaskSwitchedOut()

#endif /* FREERTOS_CONFIG_H */