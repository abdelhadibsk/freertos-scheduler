#include "scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

static sched_task_t task_table[MAX_TASKS];
static int task_count = 0;

void scheduler_init(void)
{
    task_count = 0;
}


/* Register a task with its temporal parameters */
void scheduler_register_task(TaskHandle_t task,
                             TickType_t period,
                             TickType_t deadline)
{
    configASSERT(task_count < MAX_TASKS);

    task_table[task_count].handle   = task;
    task_table[task_count].period   = period;
    task_table[task_count].deadline = deadline;

    task_count++;
}

// Apply scheduling policy by updating FreeRTOS priorities 

void scheduler_apply_policy(sched_policy_t policy){   
    
    printf("Applying scheduling policy %d\n", policy);
    /* Simple bubble sort on period/deadline */
    for (int i = 0; i < task_count - 1; i++)
    {
        for (int j = i + 1; j < task_count; j++)
        {
            int swap = 0;

            if (policy == SCHED_RM && task_table[j].period < task_table[i].period){
                swap = 1;
                printf("Swapping %d and %d based on period\n", i, j);
            }
            if (policy == SCHED_DM && task_table[j].deadline < task_table[i].deadline){
                swap = 1;
                printf("Swapping %d and %d based on deadline\n", i, j);
            }

            if (swap)
            {
                sched_task_t tmp = task_table[i];
                task_table[i] = task_table[j];
                task_table[j] = tmp;
                printf("Swapped %d and %d\n", i, j);
            }else{
                printf("No swap between %d and %d\n", i, j);
            }
        }
        printf("End of pass %d\n", i);
    }

    /* Assign priorities: highest priority = smallest index */
    for (int i = 0; i < task_count; i++)
    {   printf("Setting priority for task %d\n", i);
        UBaseType_t prio = configTASK_PRIORITY_MAX - i;
        vTaskPrioritySet(task_table[i].handle, prio);   // the problem was here
        printf("Set priority %llu for task %p\n", prio, (void*)task_table[i].handle);
    }
}
