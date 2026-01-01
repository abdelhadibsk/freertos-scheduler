#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION            1
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

#define configCHECK_FOR_STACK_OVERFLOW  2

#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0

#define configUSE_TRACE_FACILITY        1
#define configUSE_16_BIT_TICKS          0

#define configCHECK_FOR_STACK_OVERFLOW  2

#define configASSERT(x) \
    if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

#endif
