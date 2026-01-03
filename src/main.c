#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"
#include <stdio.h>

void worker_task(void *pvParameters);

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void vApplicationTickHook(void);

void MyTaskSwitchedIn(void);
void MyTaskSwitchedOut(void);

/* Add hook function prototype */
void vApplicationTaskStateHook(TaskHandle_t xTask, eTaskState eCurrentState);
void show_specific_task_states(void);

/* Add function to display all task states */
void show_all_tasks_states(void);

TaskHandle_t tA, tB, tC, tD;

int main(void)
{
    
    scheduler_init();

    // Store task handles to check states later
    xTaskCreate(worker_task, "A", 1024, "Task A", 2, &tA);
    xTaskCreate(worker_task, "B", 1024, "Task B", 1, &tB);
    xTaskCreate(worker_task, "C", 1024, "Task C", 2, &tC);
    xTaskCreate(worker_task, "D", 1024, "Task D", 1, &tD); 

    printf("Tasks created. Starting scheduler...\n");

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

// Stack overflow hook
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void) xTask;
    (void) pcTaskName;

    taskDISABLE_INTERRUPTS();
    for (;;);
}

/* Task switch in/out hooks for tracing */
void MyTaskSwitchedIn(void)
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();

    printf("[IN ] %s (prio %lu)\n",
           pcTaskGetName(h),
           (unsigned long)uxTaskPriorityGet(h));
}

void MyTaskSwitchedOut(void)
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();
    printf("[OUT] %s\n", pcTaskGetName(h));
}

/* Task state hook - called when a task's state changes */
void vApplicationTaskStateHook(TaskHandle_t xTask, eTaskState eCurrentState)
{
    const char *pcTaskName = pcTaskGetName(xTask);
    const char *pcStateString;
    
    switch(eCurrentState)
    {
        case eRunning:   pcStateString = "Running"; break;
        case eReady:     pcStateString = "Ready"; break;
        case eBlocked:   pcStateString = "Blocked"; break;
        case eSuspended: pcStateString = "Suspended"; break;
        case eDeleted:   pcStateString = "Deleted"; break;
        default:         pcStateString = "Invalid"; break;
    }
    
    printf("[STATE-HOOK] Task %s changed state to: %s\n", 
           pcTaskName ? pcTaskName : "Unknown", 
           pcStateString);
}

/* Function to display all task states */
/* Simple function to show basic task states without using uxTaskGetSystemState */

void show_specific_task_states(void)
{
    TaskHandle_t tasks[] = {tA, tB, tC, tD, scheduler_handle};
    const char* names[] = {"A", "B", "C", "D", "Scheduler"};
    
    printf("\n--- Specific Task States ---\n");
    for(int i = 0; i < 5; i++)
    {
        if(tasks[i] != NULL)
        {
            eTaskState state = eTaskGetState(tasks[i]);
            const char *state_str;
            
            switch(state)
            {
                case eRunning:   state_str = "Running"; break;
                case eReady:     state_str = "Ready"; break;
                case eBlocked:   state_str = "Blocked"; break;
                case eSuspended: state_str = "Suspended"; break;
                case eDeleted:   state_str = "Deleted"; break;
                default:         state_str = "Unknown"; break;
            }
            
            printf("Task %s: %s\n", names[i], state_str);
        }
    }
    printf("----------------------------\n");
}

// And update the tick hook to use the simpler version:
void vApplicationTickHook(void)
{
    static TickType_t tickCount = 0;
    tickCount++;
    
    if ((tickCount % 10) == 0)
    {
        printf("[TICK] %lu ticks elapsed\n", (unsigned long)tickCount);
        
        if ((tickCount % 50) == 0) 
        {
            show_specific_task_states();
        }
    }
}