#include <stdio.h>
#include "scheduler.h"
#include "task.h"


TaskHandle_t scheduler_handle;
TaskHandle_t user_tasks[TASK_COUNT];

/* USER TASK */
void worker_task(void *pvParameters)
{
    const char *name = (const char *) pvParameters;

    for (;;)
    {
        /* Wait until scheduler allows execution */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Simulated work */
        printf("%s\n", name);
        
        /* Notify scheduler that we are done */
        xTaskNotifyGive(scheduler_handle);
        taskYIELD();
    }
}

/* SCHEDULER TASK (Round Robin) */
void scheduler_task(void *pvParameters)
{
    int current = 0;

    for (;;)
    {
        /* Select next task (algorithm) */
        xTaskNotifyGive(user_tasks[current]);
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Wait until task yields back */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Round Robin decision */
        current = (current + 1) % TASK_COUNT;
    }
}
