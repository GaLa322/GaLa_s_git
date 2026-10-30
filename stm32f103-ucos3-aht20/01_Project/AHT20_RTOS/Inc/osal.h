/**
  ******************************************************************************
  * @file    osal.h
  * @brief   任务壳（Task Shell）：一个 100 行左右的时间片轮转调度器。
  *
  *          它的接口故意做成和 uC/OS 长得一样：
  *              uC/OS                 本文件
  *              OSTaskCreate()   ->   OSAL_TaskCreate()
  *              OSTimeDly()      ->   （本壳不提供阻塞延时，见 README 说明）
  *              OSStart()        ->   OSAL_Start()
  *
  *          以后把任务一（uC/OS-III 移植）做完，只要把 *_task 函数里的
  *          OSAL_TaskCreate 换成 OSTaskCreate，任务体一行都不用改。
  ******************************************************************************
  */
#ifndef __OSAL_H
#define __OSAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OSAL_MAX_TASKS      8U

typedef void (*OSAL_TaskFn)(void *arg);

/* 创建一个周期任务：每 period_ms 毫秒被调用一次，函数必须快速返回 */
int8_t   OSAL_TaskCreate(const char *name, OSAL_TaskFn fn, void *arg, uint32_t period_ms);

/* 调度器主循环，不会返回 */
void     OSAL_Start(void);

/* 清空任务表（OSAL_Start 之前调用） */
void     OSAL_Init(void);

/* 系统时基，单位 ms，来自 HAL_GetTick() */
uint32_t OSAL_Now(void);

/* 调试用：某个任务被调度的次数、当前任务数 */
uint32_t OSAL_TaskRuns(uint8_t index);
uint8_t  OSAL_TaskCount(void);

#ifdef __cplusplus
}
#endif

#endif /* __OSAL_H */
