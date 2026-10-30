/**
  ******************************************************************************
  * @file    app_tasks.h
  * @brief   三个应用任务：LED1(1s) / LED2(3s) / AHT20(2s)
  ******************************************************************************
  */
#ifndef __APP_TASKS_H
#define __APP_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 AHT20、打印开机横幅、创建 3 个任务。OSAL_Start() 之前调用。 */
void App_TasksInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TASKS_H */
