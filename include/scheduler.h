#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"

/* Number of user tasks */
#define TASK_COUNT 3

extern TaskHandle_t scheduler_handle;
extern TaskHandle_t user_tasks[TASK_COUNT];

/* Scheduler task */
void scheduler_task(void *pvParameters);

/* Worker task */
void worker_task(void *pvParameters);

#endif
