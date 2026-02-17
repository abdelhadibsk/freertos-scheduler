#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"
#include <stdio.h>

void worker_task(void *pvParameters);
void periodic_task(void *pvParameters);
void init_task(void *p);

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void vApplicationTickHook(void);

void MyTaskSwitchedIn(void);
void MyTaskSwitchedOut(void);

TaskHandle_t tA, tB, tC, tD;

int main(void)
{   
    // at the new model i should not need to pass the task parameters to the task function since they are stored in the TCB
    // we can access them directly from the TCB using the task handle, cuz handle is a pointer to the TCB, so we can define a struct for the task parameters and store it in the TCB when creating the task, and then access it directly in the task function using the handle. This way we can avoid passing the parameters as arguments to the task function and keep the code cleaner.

    // but for simplicity i will pass them as parameters to the task function in this example, and we can later modify the code to access them directly from the TCB in the task function if needed. This way we can keep the task function simple and focused on its execution logic without worrying about how to access its parameters.

    printf("MAIN START\n");
    setvbuf(stdout, NULL, _IONBF, 0);

    scheduler_init();   //count 0

    // Create user tasks
    sched_task_t taskA, taskB, taskC;

    // tasks parameters
    taskA.period    = pdMS_TO_TICKS(100);   // Ti = 100ms
    taskA.deadline  = pdMS_TO_TICKS(100);   // Di = 100ms
    taskA.exec_time = pdMS_TO_TICKS(20);    // Ci = 20ms
    taskA.handle = tA;

    taskB.period    = pdMS_TO_TICKS(200);
    taskB.deadline  = pdMS_TO_TICKS(200);
    taskB.exec_time = pdMS_TO_TICKS(40);
    taskB.handle = tB;

    taskC.period    = pdMS_TO_TICKS(400);   
    taskC.deadline  = pdMS_TO_TICKS(400);
    taskC.exec_time = pdMS_TO_TICKS(60);
    taskC.handle = tC;

    // we can add a hook function to xTaskCreate to add this parameters directly when creating the task
    // we can add this infos to a config struct passed to the task in scheduler.h or we can create a wrapper around xTaskCreate to do this in one step

    // trying to use only the handle to access the task parameters in the task function, so we will not pass the parameters as arguments to the task function, but we will store them in the TCB when creating the task, and then access them directly in the task function using the handle.
    // the problem now is that we can t acces the TCB directly since it is not exposed in the FreeRTOS API, 
    xTaskCreate(periodic_task, "A", 1024, &taskA, 1, &tA); 
    xTaskCreate(periodic_task, "B", 1024, &taskB, 1, &tB);
    xTaskCreate(periodic_task, "C", 1024, &taskC, 1, &tC);
    printf("tasks created\n");
    // print tasks info
    fflush(stdout);


    printf("Task A: period=%lu, deadline=%lu, exec_time=%lu\n", taskA.period, taskA.deadline, taskA.exec_time);
    printf("Task B: period=%lu, deadline=%lu, exec_time=%lu\n", taskB.period, taskB.deadline, taskB.exec_time);
    printf("Task C: period=%lu, deadline=%lu, exec_time=%lu\n", taskC.period, taskC.deadline, taskC.exec_time);

    fflush(stdout);
    // replace this four lines by a function canlled inside xTaskCreate to register the task directly when creating it 
    
    scheduler_register_task(tA, taskA.period, taskA.deadline);
    scheduler_register_task(tB, taskB.period, taskB.deadline);
    scheduler_register_task(tC, taskC.period, taskC.deadline);
    printf("tasks registered\n");
    fflush(stdout);

    xTaskCreate(init_task, "Init", 1024, NULL, configSCHEDULER_PRIORITY, NULL);
    printf("Starting scheduler\n");
    fflush(stdout);

    vTaskStartScheduler();  // This should never return
    for (;;);
}

void init_task(void *p)
{
    printf("INIT TASK START\n");

    // Choose scheduling policy 
    // scheduler_apply_policy(SCHED_RM);
    // scheduler_apply_policy(SCHED_DM);
    // scheduler_apply_policy(SCHED_FIFO);

    scheduler_apply_policy(SCHED_RM);

    printf("INIT TASK DONE\n");
    vTaskDelete(NULL);
    
}

// Simple worker task (not periodic, for testing)
void worker_task(void *pvParameters)    
{
    const char *name = (const char *)pvParameters;

    for (;;)
    {
        printf("Task %s running\n", name);
        vTaskDelay(pdMS_TO_TICKS(100)); // to simulate work
    }
}
// task apériodique isr , etats 

// PERIODIC TASK WITH CONTROLLED EXECUTION TIME 
void periodic_task(void *pvParameters)
{
    // sched_task_t *task = (sched_task_t *)pvParameters;   //this line is not really necessary since we can access the task parameters directly from the TCB using the task handle, but it simplifies the code for this example
    //explaining the line above: we pass the address of the sched_task_t struct as the parameter when creating the task, so we can cast the void* parameter to a sched_task_t* to access the task parameters directly in the task function. This is a common pattern in FreeRTOS to pass multiple parameters to a task through a struct.

    // trynig to access the task parameters directly from the TCB using the task handle, we will not use sched_task struct in the task function,     
    
    // sched_task_t *task = (sched_task_t *)pvParameters;
    sched_task_t *task = (sched_task_t *)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
 
    TaskParameters_t *pxTaskParameters = (TaskParameters_t *)pvParameters;


    for (;;)
    {
        /* ===== Activation périodique stricte ===== */
        vTaskDelayUntil(&lastWakeTime, task->period);
        task->last_release = lastWakeTime;

        /* ===== Début du job ===== */
        //printf("[JOB START] Task %s at %lu\n", pcTaskGetName(NULL), (unsigned long)xTaskGetTickCount());

        TickType_t exec_start = xTaskGetTickCount();

        /* ===== Exécution contrôlée ===== */
        while ((xTaskGetTickCount() - exec_start) < task->exec_time)
        {   // simulate work
            //taskYIELD();  // permet la préemption utile cas de priorité égale
        }

        /* ===== Fin du job ===== */
        TickType_t finish = xTaskGetTickCount();

        //printf("[JOB END] Task %s at %lu (exec = %lu)\n",pcTaskGetName(NULL), (unsigned long)finish, (unsigned long)(finish - exec_start));
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

// Task switch in/out hooks for tracing 
void MyTaskSwitchedIn(void)
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();

    printf("[IN ] %s at %lu\n", pcTaskGetName(h), (unsigned long)xTaskGetTickCount());

}

void MyTaskSwitchedOut(void)
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();
    printf("[OUT] %s at %lu\n", pcTaskGetName(h), (unsigned long)xTaskGetTickCount());
}

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

