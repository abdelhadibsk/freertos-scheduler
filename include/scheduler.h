#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "list.h"

#define SCHEDULER_PRIORITY          (configMAX_PRIORITIES - 1)

typedef struct
{
    TaskHandle_t handle;
    ListItem_t  list_item;
} sched_task_t;

extern TaskHandle_t scheduler_handle;

void scheduler_register_task(TaskHandle_t task);
void scheduler_task(void *pvParameters);
void scheduler_preempt_task(void *pvParameters);
void scheduler_no_preempt_task(void *pvParameters);



#endif
