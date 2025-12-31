#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

int main(void)
{
    /* Create scheduler task (higher priority) */
    xTaskCreate(scheduler_task, "SCHED", 1024, NULL, configMAX_PRIORITIES, &scheduler_handle);

    /* Create user tasks (same priority) */
    xTaskCreate(worker_task, "A", 512, "Task A", 1, &user_tasks[0]);
    xTaskCreate(worker_task, "B", 512, "Task B", 1, &user_tasks[1]);
    xTaskCreate(worker_task, "C", 512, "Task C", 1, &user_tasks[2]);

    /* Start FreeRTOS */
    vTaskStartScheduler();

    for (;;);
}
