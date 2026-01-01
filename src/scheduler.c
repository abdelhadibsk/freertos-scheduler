#include <stdio.h>
#include "scheduler.h"
#include "task.h"

static List_t sched_task_list;
TaskHandle_t scheduler_handle;
static BaseType_t scheduler_initialized = pdFALSE;

static TaskHandle_t current_task = NULL;
static List_t ready_list; // from the other branch

void scheduler_init(void)   // other branch
{
    vListInitialise(&ready_list);
    xTaskCreate(scheduler_task, "Scheduler", configMINIMAL_STACK_SIZE, NULL, SCHEDULER_PRIORITY, NULL);
}

/* Register a task with the scheduler and suspend it until scheduled */
void scheduler_register_task(TaskHandle_t task)
{
    while (scheduler_initialized == pdFALSE)
    {
        vTaskDelay(pdMS_TO_TICKS(1));   // wait until scheduler initializes list
    }

    sched_task_t *entry = pvPortMalloc(sizeof(sched_task_t));
    configASSERT(entry);

    entry->handle = task;
    vListInitialiseItem(&entry->list_item);
    listSET_LIST_ITEM_OWNER(&entry->list_item, entry);

    vListInsertEnd(&sched_task_list, &entry->list_item);
    vTaskSuspend(task);
} 

/* Scheduler task : this function implements a simple round-robin scheduler, 
allowing each registered task to run for a time slice of 500 ms */
void scheduler_task(void *pvParameters)
{
    vListInitialise(&sched_task_list);
    scheduler_initialized = pdTRUE;

    vTaskDelay(pdMS_TO_TICKS(10)); // let system start duration : 10 ms

    for (;;)
    {   // if no registered tasks, wait
        if (listLIST_IS_EMPTY(&sched_task_list))
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;   // the line will jump to the beginning of the for loop
        }

        /* Pick next task (round-robin) */
        ListItem_t *current_item = listGET_HEAD_ENTRY(&sched_task_list);
        sched_task_t *next_task = (sched_task_t *) listGET_LIST_ITEM_OWNER(current_item);

        /* Suspend previous task only */
        if (current_task != NULL)
            vTaskSuspend(current_task);

        /* Notify and resume next task (unblock worker waiting on notification) */
        xTaskNotifyGive(next_task->handle);
        vTaskResume(next_task->handle);
        current_task = next_task->handle;

        /* Wait until worker notifies back, or time slice expires */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));

        /* Rotate list */
        uxListRemove(current_item);
        vListInsertEnd(&sched_task_list, current_item);
    }
}

/* Scheduler simple preemption implementation excute the hiest priority task immediately */
void scheduler_preempt_task(void *pvParameters)
{
    vListInitialise(&sched_task_list);
    scheduler_initialized = pdTRUE;

    vTaskDelay(pdMS_TO_TICKS(100)); // let system start

    for (;;)
    {
        if (listLIST_IS_EMPTY(&sched_task_list))
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
    
        /* Pick highest priority task */
        ListItem_t *current_item = listGET_HEAD_ENTRY(&sched_task_list);
        sched_task_t *next_task = (sched_task_t *) listGET_LIST_ITEM_OWNER(current_item);

        /* Suspend previous task only */
        if (current_task != NULL)
            vTaskSuspend(current_task);
        
        /* Notify and resume next task (unblock worker waiting on notification) */
        xTaskNotifyGive(next_task->handle);
        vTaskResume(next_task->handle);
        current_task = next_task->handle;

        /* Wait until worker notifies back, or time slice expires */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));

        /* Rotate list */
        uxListRemove(current_item);
        vListInsertEnd(&sched_task_list, current_item);
    }
}

/* Scheduler no preemption implementation */
void scheduler_no_preempt_task(void *pvParameters)
{
    vListInitialise(&sched_task_list);
    scheduler_initialized = pdTRUE;

    vTaskDelay(pdMS_TO_TICKS(100)); // let system start

    for (;;)
    {
        if (listLIST_IS_EMPTY(&sched_task_list))
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* Pick highest priority task */
        ListItem_t *current_item = listGET_HEAD_ENTRY(&sched_task_list);
        sched_task_t *next_task = (sched_task_t *) listGET_LIST_ITEM_OWNER(current_item);

        /* Suspend previous task only */
        if (current_task != NULL)
            vTaskSuspend(current_task);     

        /* Notify and resume next task (unblock worker waiting on notification) */
        xTaskNotifyGive(next_task->handle); 
        vTaskResume(next_task->handle);
        current_task = next_task->handle;

        /* Wait until worker notifies back */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Rotate list */
        uxListRemove(current_item);
        vListInsertEnd(&sched_task_list, current_item);


    }
}

