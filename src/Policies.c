/* =========================================================
 * policies.c
 * RT Scheduling Policies — RM, DM, FIFO, EDF
 *
 * Each policy implements one function:
 *   vRM_UpdatePriorities()
 *   vDM_UpdatePriorities()
 *   vFIFO_UpdatePriorities()
 *   vEDF_UpdatePriorities()
 *
 * vApplicationSchedulerUpdatePriorities() in scheduler.c
 * calls the correct one based on the config in scheduler.h.
 *
 * All policies use only the helper API — they never touch
 * the TCB or FreeRTOS internals directly.
 *
 * Ranking logic (same pattern for all policies):
 *   For each task i, count how many other tasks rank above it.
 *   That count becomes its rank (0 = highest priority).
 *   FreeRTOS priority = (configMAX_PRIORITIES - 2) - rank
 * ========================================================= */

#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

/* =========================================================
 * Shared helper: assigns priorities by rank array
 * rank[i] = 0 → highest FreeRTOS priority
 * rank[i] = n-1 → lowest FreeRTOS priority
 * ========================================================= */
static void prvApplyRanks( UBaseType_t * ranks, UBaseType_t n )
{
    UBaseType_t i;

    for( i = 0; i < n; i++ )
    {
        TaskHandle_t xTask = xRTGetTaskByIndex( i );
        UBaseType_t  uxPriority = ( configMAX_PRIORITIES - 2U ) - ranks[ i ];

        /* Safety clamp */
        if( uxPriority < ( tskIDLE_PRIORITY + 1U ) )
            uxPriority = tskIDLE_PRIORITY + 1U;

        vRTSetTaskPriority( xTask, uxPriority );
    }
}

/* =========================================================
 * RM — Rate Monotonic
 *
 * Rule    : Shorter period → Higher priority (static)
 * Theory  : Optimal fixed-priority policy for implicit deadlines
 * Bound   : U = sum(Ci/Ti) <= n*(2^(1/n) - 1)  →  ln(2) ≈ 0.693
 * ========================================================= */
void vRM_UpdatePriorities( void )
{
    UBaseType_t n = uxRTGetTaskCount();
    UBaseType_t ranks[ configMAX_RT_TASKS ] = { 0 };
    UBaseType_t i, j;

    if( n == 0 ) return;

    for( i = 0; i < n; i++ )
    {
        TickType_t periodI = xRTGetTaskPeriod( xRTGetTaskByIndex( i ) );

        for( j = 0; j < n; j++ )
        {
            if( j == i ) continue;
            TickType_t periodJ = xRTGetTaskPeriod( xRTGetTaskByIndex( j ) );

            /* j ranks above i if j has shorter period */
            if( periodJ < periodI )                           ranks[ i ]++;
            /* Tie-break: lower index wins */
            else if( ( periodJ == periodI ) && ( j < i ) )   ranks[ i ]++;
        }
    }

    prvApplyRanks( ranks, n );
}

/* =========================================================
 * DM — Deadline Monotonic
 *
 * Rule    : Shorter relative deadline → Higher priority (static)
 * Theory  : Optimal fixed-priority policy for constrained deadlines
 * Note    : Reduces to RM when Di == Ti
 * ========================================================= */
void vDM_UpdatePriorities( void )
{
    UBaseType_t n = uxRTGetTaskCount();
    UBaseType_t ranks[ configMAX_RT_TASKS ] = { 0 };
    UBaseType_t i, j;

    if( n == 0 ) return;

    for( i = 0; i < n; i++ )
    {
        TickType_t deadlineI = xRTGetTaskDeadline( xRTGetTaskByIndex( i ) );

        for( j = 0; j < n; j++ )
        {
            if( j == i ) continue;
            TickType_t deadlineJ = xRTGetTaskDeadline( xRTGetTaskByIndex( j ) );

            if( deadlineJ < deadlineI )                           ranks[ i ]++;
            else if( ( deadlineJ == deadlineI ) && ( j < i ) )   ranks[ i ]++;
        }
    }

    prvApplyRanks( ranks, n );
}

/* =========================================================
 * FIFO — First In First Out
 *
 * Rule    : First released job → Highest priority (dynamic)
 * Order   : Based on release_order counter (set by TickHook)
 * Note    : Non-ready tasks get lowest priority
 * ========================================================= */
void vFIFO_UpdatePriorities( void )
{
    UBaseType_t n = uxRTGetTaskCount();
    UBaseType_t ranks[ configMAX_RT_TASKS ] = { 0 };
    UBaseType_t i, j;

    if( n == 0 ) return;

    for( i = 0; i < n; i++ )
    {
        TaskHandle_t xTaskI = xRTGetTaskByIndex( i );

        /* Non-ready tasks get pushed to lowest priority */
        if( !xRTIsTaskReady( xTaskI ) )
        {
            ranks[ i ] = n;   /* Will clamp to minimum in prvApplyRanks */
            continue;
        }

        UBaseType_t orderI = uxRTGetTaskReleaseOrder( xTaskI );

        for( j = 0; j < n; j++ )
        {
            if( j == i ) continue;
            TaskHandle_t xTaskJ = xRTGetTaskByIndex( j );

            if( !xRTIsTaskReady( xTaskJ ) ) continue;

            UBaseType_t orderJ = uxRTGetTaskReleaseOrder( xTaskJ );

            /* j ranks above i if j was released earlier */
            if( orderJ < orderI )                           ranks[ i ]++;
            else if( ( orderJ == orderI ) && ( j < i ) )   ranks[ i ]++;
        }
    }

    prvApplyRanks( ranks, n );
}

/* =========================================================
 * EDF — Earliest Deadline First  (Future)
 *
 * Rule    : Smallest absolute_deadline → Highest priority (dynamic)
 * Theory  : Optimal online algorithm — can schedule any set with U <= 1
 * Note    : absolute_deadline updated every job release in TickHook
 * 
 * This version of EDF is non-work-conserving: if a task misses its deadline and is still ready, it will keep the same absolute_deadline and thus the same priority, allowing other tasks to run. A work-conserving version would need to update absolute_deadline on every tick for ready tasks, which would be more complex and costly.
 * Change the logic in vEDF_UpdatePriorities if you want a work-conserving version.
 * Possible improvement: maintain a sorted list of ready tasks by absolute_deadline to avoid O(n^2) ranking.
 * ========================================================= */
void vEDF_UpdatePriorities( void )
{
    UBaseType_t n = uxRTGetTaskCount();
    UBaseType_t ranks[ configMAX_RT_TASKS ] = { 0 };
    UBaseType_t i, j;

    if( n == 0 ) return;

    for( i = 0; i < n; i++ )
    {
        TaskHandle_t xTaskI = xRTGetTaskByIndex( i );

        if( !xRTIsTaskReady( xTaskI ) )
        {
            ranks[ i ] = n;
            continue;
        }

        TickType_t absDeadI = xRTGetTaskAbsoluteDeadline( xTaskI );

        for( j = 0; j < n; j++ )
        {
            if( j == i ) continue;
            TaskHandle_t xTaskJ = xRTGetTaskByIndex( j );

            if( !xRTIsTaskReady( xTaskJ ) ) continue;

            TickType_t absDeadJ = xRTGetTaskAbsoluteDeadline( xTaskJ );

            /* j ranks above i if j has an earlier absolute deadline */
            if( absDeadJ < absDeadI )                           ranks[ i ]++;
            else if( ( absDeadJ == absDeadI ) && ( j < i ) )   ranks[ i ]++;
        }
    }

    prvApplyRanks( ranks, n );
}