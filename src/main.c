#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

void worker_task(void *pvParameters);
void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName);

int main(void)
{
    TaskHandle_t t;

    xTaskCreate(scheduler_task, "SCHED", 1024, NULL,
                configMAX_PRIORITIES - 1, &scheduler_handle);

    xTaskCreate(worker_task, "A", 1024, "Task A", 1, &t);
    scheduler_register_task(t);

    xTaskCreate(worker_task, "B", 1024, "Task B", 1, &t);
    scheduler_register_task(t);

    xTaskCreate(worker_task, "C", 1024, "Task C", 1, &t);
    scheduler_register_task(t);

    vTaskStartScheduler();
    for (;;);
}


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
        //vTaskDelay(pdMS_TO_TICKS(500));

        /* Notify scheduler that we are done */
        xTaskNotifyGive(scheduler_handle);
        taskYIELD();
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    (void) xTask;
    (void) pcTaskName;

    taskDISABLE_INTERRUPTS();
    for (;;);
}