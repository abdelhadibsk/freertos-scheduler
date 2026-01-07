#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"
#include <stdio.h>

void worker_task(void *pvParameters);
void init_task(void *p);

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
    printf("MAIN START\n");
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("STDOUT UNBUFFERED\n");

    scheduler_init();

    xTaskCreate(worker_task, "A", 1024, "A", 1, &tA);
    xTaskCreate(worker_task, "B", 1024, "B", 1, &tB);
    xTaskCreate(worker_task, "C", 1024, "C", 1, &tC);
    printf("tasks created\n");
    fflush(stdout);


    scheduler_register_task(tA, 100, 100);
    scheduler_register_task(tB, 200, 200);
    scheduler_register_task(tC, 400, 400);
    printf("tasks registered\n");
    fflush(stdout);

    /* Choose scheduling policy */
    // scheduler_apply_policy(SCHED_RM);
    // printf("policy applied\n");
    // fflush(stdout);
    // scheduler_apply_policy(SCHED_DM);
    // scheduler_apply_policy(SCHED_FIFO);

    xTaskCreate(init_task, "Init", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
    printf("Starting scheduler\n");
    fflush(stdout);

    vTaskStartScheduler();
    for (;;);
}

void init_task(void *p)
{
    printf("INIT TASK START\n");

    scheduler_apply_policy(SCHED_RM);

    printf("INIT TASK DONE\n");
    vTaskDelete(NULL);
}


/* USER TASK */
void worker_task(void *pvParameters)
{
    const char *name = (const char *)pvParameters;

    for (;;)
    {
        printf("Task %s running\n", name);
        vTaskDelay(pdMS_TO_TICKS(100));
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


/*
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
*/

// And update the tick hook to use the simpler version:

void vApplicationTickHook(void)
{
    static TickType_t tickCount = 0;
    tickCount++;
    
    if ((tickCount % 100) == 0)
    {
        printf("[TICK] %lu ticks elapsed\n", (unsigned long)tickCount);
        /*
        if ((tickCount % 100) == 0) 
        {
            show_specific_task_states();
        }
        */    
    }
}