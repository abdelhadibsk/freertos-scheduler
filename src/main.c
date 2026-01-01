#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

void worker_task(void *pvParameters);
void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName);

int main(void)
{
    TaskHandle_t t;
    // testing the rond-robin scheduler    
    // xTaskCreate(scheduler_task, "SCHED", 1024, NULL, configMAX_PRIORITIES - 1, &scheduler_handle);   

    // testing preemptive scheduler
    //xTaskCreate(scheduler_preempt_task, "SCHED", 1024, NULL, configMAX_PRIORITIES - 1, &scheduler_handle);

    // testing non-preemptive scheduler
    xTaskCreate(scheduler_no_preempt_task, "SCHED", 1024, NULL, configMAX_PRIORITIES - 1, &scheduler_handle);
    
    xTaskCreate(worker_task, "A", 1024, "Task A", 2, &t);
    xTaskCreate(worker_task, "B", 1024, "Task B", 1, &t);
    xTaskCreate(worker_task, "C", 1024, "Task C", 4, &t);
    xTaskCreate(worker_task, "D", 1024, "Task D", 8, &t); // higher priority

    vTaskStartScheduler();
    for (;;);
} 


/* USER TASK */
void worker_task(void *pvParameters)
{
    const char *name = (const char *) pvParameters;

    /* Register self with scheduler (will block until scheduler initializes and then suspend the task) */
    scheduler_register_task(xTaskGetCurrentTaskHandle());

    for (;;)
    {
        /* Wait until scheduler allows execution */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Simulated work */
        printf("%s\n", name);
        vTaskDelay(pdMS_TO_TICKS(500));

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