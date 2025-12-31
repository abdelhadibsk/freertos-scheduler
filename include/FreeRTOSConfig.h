#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define INCLUDE_vTaskDelay              1
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          0
#define configCPU_CLOCK_HZ              ( ( unsigned long ) 100000000 )
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 )

#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128
#define configTOTAL_HEAP_SIZE           ( 20 * 1024 )

#define configUSE_TASK_NOTIFICATIONS    1
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0

#define configUSE_TRACE_FACILITY        0
#define configUSE_16_BIT_TICKS          0

#endif
