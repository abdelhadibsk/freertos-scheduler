#include <stdio.h>
#include "scheduler.h"
#include "task.h"

static List_t sched_task_list;
static BaseType_t scheduler_initialized = pdFALSE;

static List_t sched_task_list;
TaskHandle_t scheduler_handle;

static TaskHandle_t current_task = NULL;

void scheduler_register_task(TaskHandle_t task)
{
    while (scheduler_initialized == pdFALSE)
    {
        taskYIELD();   // wait until scheduler initializes list
    }

    sched_task_t *entry = pvPortMalloc(sizeof(sched_task_t));
    configASSERT(entry);

    entry->handle = task;
    vListInitialiseItem(&entry->list_item);
    listSET_LIST_ITEM_OWNER(&entry->list_item, entry);

    vListInsertEnd(&sched_task_list, &entry->list_item);
    vTaskSuspend(task);
}


void scheduler_task(void *pvParameters)
{
    vListInitialise(&sched_task_list);
    scheduler_initialized = pdTRUE;

    vTaskDelay(pdMS_TO_TICKS(100)); // let system start

    for (;;)
    {
        if (listLIST_IS_EMPTY(&sched_task_list))
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        /* Pick next task (round-robin) */
        ListItem_t *current_item = listGET_HEAD_ENTRY(&sched_task_list);
        sched_task_t *next_task = (sched_task_t *) listGET_LIST_ITEM_OWNER(current_item);

        /* Suspend previous task only */
        if (current_task != NULL)
            vTaskSuspend(current_task);

        /* Resume next task */
        vTaskResume(next_task->handle);
        current_task = next_task->handle;

        /* Force context switch */
        taskYIELD();

        /* Time slice */
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Rotate list */
        uxListRemove(current_item);
        vListInsertEnd(&sched_task_list, current_item);
    }
}
