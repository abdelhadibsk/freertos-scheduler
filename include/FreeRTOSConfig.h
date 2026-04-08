#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ================= CORE SCHEDULING ================= */

#define configUSE_PREEMPTION            1   /* IMPORTANT */
#define configUSE_TIME_SLICING          1   /* No time slicing: tasks run until they block or yield */
#define configRUN_MULTIPLE_PRIORITIES   1
#define configNUMBER_OF_CORES           1

/* ================= Scheduling algorithms ================= */

#define configUSE_SCHEDULER 1

/* periodique tasks */
#define configUSE_PERIODIC_TASKS              1   // to enable regestering the parameters of periodic tasks in scheduler_register_task

/* ================= CPU & TICK ================= */

#define configCPU_CLOCK_HZ              ( ( unsigned long ) 100000000 ) // 10 MHz CPU clock 
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 ) /* 1ms tick */
#define configUSE_16_BIT_TICKS          0

/* ================= TASKS ================= */

#define configMAX_PRIORITIES            10  // max number of tasks
#define configMINIMAL_STACK_SIZE        256
#define configTOTAL_HEAP_SIZE           ( 20 * 1024 )
#define configUSE_TASK_NOTIFICATIONS    1 

/* ================= HOOKS ================= */

#define configCHECK_FOR_STACK_OVERFLOW  0   // to enable vApplicationStackOverflowHook
#define configUSE_IDLE_HOOK             0   // to enable vApplicationIdleHook
#define configUSE_TICK_HOOK             0   // to enable vApplicationTickHook
#define configUSE_TASK_STATE_HOOK       0   // to enable vApplicationTaskStateHook



/* ================= TRACE (pour [IN]/[OUT]) ================= */

#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

#define traceTASK_SWITCHED_IN()   MyTaskSwitchedIn()
#define traceTASK_SWITCHED_OUT()  MyTaskSwitchedOut()

/* ================= API INCLUSION ================= */

#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_vTaskResume             1
// #define INCLUDE_vTaskDelayUntil      1   // not found in task.c
#define INCLUDE_xTaskDelayUntil         1

#define INCLUDE_uxTaskPriorityGet       1   //to use uxTaskPriorityGet in MyTaskSwitchedIn/Out
#define INCLUDE_vTaskPrioritySet        1   //to use vTaskPrioritySet in scheduler_apply_policy
#define INCLUDE_vTaskDelete             1

#define INCLUDE_xTaskResumeFromISR  1   // to use xTaskResumeFromISR in vApplicationSchedulerTickHook

/* ================= ASSERT ================= */

#define configASSERT(x) \
    if ((x) == 0) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#endif /* FREERTOS_CONFIG_H */