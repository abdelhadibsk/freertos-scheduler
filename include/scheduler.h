#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"

#define MAX_TASKS  8
//define scheduler prioritie
#define configSCHEDULER_PRIORITY  ( configMAX_PRIORITIES - 1 )
//define task priority max
#define configTASK_PRIORITY_MAX   ( configSCHEDULER_PRIORITY - 1 )

typedef enum {
    SCHED_FIFO,
    SCHED_RM,
    SCHED_DM
} sched_policy_t;

typedef struct {
    TaskHandle_t handle;
    TickType_t   period;
    TickType_t   deadline;
    TickType_t   wcet; // worst-case execution time to be added later
} sched_task_t;

/* API */
void scheduler_init(void);
void scheduler_register_task(TaskHandle_t task,
                             TickType_t period,
                             TickType_t deadline);
void scheduler_apply_policy(sched_policy_t policy);

#endif
