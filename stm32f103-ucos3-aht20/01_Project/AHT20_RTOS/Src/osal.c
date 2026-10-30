/**
  ******************************************************************************
  * @file    osal.c
  * @brief   任务壳的实现：SysTick(1ms) 计时 + 主循环里按周期派发。
  *
  *          和"裸机 while(1) + HAL_Delay"最大的区别：
  *            每个任务只做一小步就返回，谁也不许霸占 CPU，
  *            所以三个周期（1s / 3s / 2s）才会同时成立。
  ******************************************************************************
  */
#include "osal.h"
#include "main.h"
#include <string.h>

typedef struct
{
    const char   *name;
    OSAL_TaskFn   fn;
    void         *arg;
    uint32_t      period_ms;
    uint32_t      last_ms;
    uint32_t      runs;
} OSAL_Task;

static OSAL_Task s_tasks[OSAL_MAX_TASKS];
static uint8_t   s_task_count = 0U;

void OSAL_Init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_task_count = 0U;
}

uint32_t OSAL_Now(void)
{
    return HAL_GetTick();
}

int8_t OSAL_TaskCreate(const char *name, OSAL_TaskFn fn, void *arg, uint32_t period_ms)
{
    OSAL_Task *task;

    if ((fn == 0) || (period_ms == 0U) || (s_task_count >= OSAL_MAX_TASKS))
    {
        return -1;
    }

    task = &s_tasks[s_task_count];
    task->name      = name;
    task->fn        = fn;
    task->arg       = arg;
    task->period_ms = period_ms;
    task->last_ms   = HAL_GetTick() - period_ms;  /* 让任务在启动后立刻跑第一次 */
    task->runs      = 0U;

    s_task_count++;
    return (int8_t)(s_task_count - 1U);
}

void OSAL_Start(void)
{
    for (;;)
    {
        uint8_t  ran = 0U;
        uint8_t  i;
        uint32_t now = HAL_GetTick();

        for (i = 0U; i < s_task_count; i++)
        {
            OSAL_Task *task = &s_tasks[i];

            if ((uint32_t)(now - task->last_ms) >= task->period_ms)
            {
                /* 固定节拍：last_ms 按周期累加，长任务不会让周期整体漂移 */
                task->last_ms += task->period_ms;
                if ((uint32_t)(now - task->last_ms) >= task->period_ms)
                {
                    task->last_ms = now;   /* 落后太多就直接对齐，避免"补跑"风暴 */
                }

                task->runs++;
                task->fn(task->arg);

                ran = 1U;
                now = HAL_GetTick();       /* 任务里可能耗时，重新取时间 */
            }
        }

        if (ran == 0U)
        {
            __WFI();                       /* 没事干就睡一觉，等 SysTick 把人叫醒 */
        }
    }
}

uint32_t OSAL_TaskRuns(uint8_t index)
{
    if (index >= s_task_count)
    {
        return 0U;
    }
    return s_tasks[index].runs;
}

uint8_t OSAL_TaskCount(void)
{
    return s_task_count;
}
