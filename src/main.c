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

// changing heap_3.c to heap_1.c causes malloc to fail and tasks not to be created, which is expected since heap_1 does not support freeing memory and we are creating multiple tasks that require dynamic allocation.
// using static allocation for tasks (configSUPPORT_STATIC_ALLOCATION) allows us to create tasks without relying on the heap, which is why it works even with heap_1.c. However, we need to ensure that we provide the necessary static buffers for the task control blocks and stacks when using static allocation.

static StaticTask_t xTaskTCB1, xTaskTCB2, xTaskTCB3;
static StackType_t xStack1[ configMINIMAL_STACK_SIZE ];
static StackType_t xStack2[ configMINIMAL_STACK_SIZE ];
static StackType_t xStack3[ configMINIMAL_STACK_SIZE ];

int main(void)
{   
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

    
    tA = xTaskCreateStatic(periodic_task, "A", 1024, &taskA, 1, xStack1, &xTaskTCB1); 
    tB = xTaskCreateStatic(periodic_task, "B", 1024, &taskB, 1, xStack2, &xTaskTCB2);
    tC = xTaskCreateStatic(periodic_task, "C", 1024, &taskC, 1, xStack3, &xTaskTCB3);
    printf("tasks created\n");
    
    vTaskSuspend(tA);
    vTaskSuspend(tB);
    vTaskSuspend(tC);
    
    // print tasks info
    fflush(stdout);


    printf("Task A: period=%lu, deadline=%lu, exec_time=%lu\n", taskA.period, taskA.deadline, taskA.exec_time);
    printf("Task B: period=%lu, deadline=%lu, exec_time=%lu\n", taskB.period, taskB.deadline, taskB.exec_time);
    printf("Task C: period=%lu, deadline=%lu, exec_time=%lu\n", taskC.period, taskC.deadline, taskC.exec_time);

    fflush(stdout);

    scheduler_register_task(tA, taskA.period, taskA.deadline);
    scheduler_register_task(tB, taskB.period, taskB.deadline);
    scheduler_register_task(tC, taskC.period, taskC.deadline);
    printf("tasks registered\n");
    fflush(stdout);

    TaskHandle_t tInit = xTaskCreateStatic(init_task, "Init", 1024, NULL, configSCHEDULER_PRIORITY, xStack1, &xTaskTCB1);
    printf("Starting scheduler\n");
    fflush(stdout);

    vTaskStartScheduler();  // This should never return
    for (;;);
}

void init_task(void *p)
{
    printf("INIT TASK START\n");
    fflush(stdout);

    // Choose scheduling policy 
    // scheduler_apply_policy(SCHED_RM);
    // scheduler_apply_policy(SCHED_DM);
    // scheduler_apply_policy(SCHED_FIFO);

    scheduler_apply_policy(SCHED_RM);
    vTaskResume(tA);
    vTaskResume(tB);    
    vTaskResume(tC);

    printf("INIT TASK DONE\n");
    fflush(stdout);
    // vTaskDelete(NULL);
    // vTaskPrioritySet(NULL, 0); // lower priority to let other tasks run
    vTaskSuspend(NULL); // suspend itself
    
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
    sched_task_t *task = (sched_task_t *)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();

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

