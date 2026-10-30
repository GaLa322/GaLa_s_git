/**
  ******************************************************************************
  * @file    app_tasks.c
  * @brief   uC/OS-III 版本：三个任务（任务一 + 任务二第 3 小问）
  *
  *  任务         优先级   周期       作用
  *  ----------   ------   --------   ----------------------------------------
  *  Task_Led1      6      1000 ms    翻转 LED1（PC13 板载）
  *  Task_Led2      7      3000 ms    翻转 LED2（PB12 外接）
  *  Task_Aht20     8      2000 ms    打印 hello uc/OS! + 采集 AHT20 温湿度
  *
  *  与 AHT20_RTOS（OSAL 版）的区别只有两处：
  *    1. 创建任务：OSAL_TaskCreate  -> OSTaskCreate（要自己给 TCB 和栈）
  *    2. 周期延时：任务函数返回      -> OSTimeDlyHMSM（主动阻塞，CPU 让给别人）
  *  任务体（点灯、打印、采集）一行没变。
  ******************************************************************************
  */
#include "app_tasks.h"
#include "board_config.h"
#include "bsp_uart.h"
#include "aht20.h"
#include "main.h"
#include "os.h"
#include "diag.h"
#include <stdio.h>

/* ------------------------------------------------------------- 内核对象 */
#define TASK_LED1_PRIO      6u
#define TASK_LED2_PRIO      7u
#define TASK_AHT_PRIO       8u

#define TASK_LED1_STK_SIZE  128u    /* 单位：CPU_STK（4 字节） */
#define TASK_LED2_STK_SIZE  128u
#define TASK_AHT_STK_SIZE   384u    /* 有 vsnprintf + 浮点，栈要给足 */

static OS_TCB  s_led1_tcb;
static OS_TCB  s_led2_tcb;
static OS_TCB  s_aht_tcb;

static CPU_STK s_led1_stk[TASK_LED1_STK_SIZE];
static CPU_STK s_led2_stk[TASK_LED2_STK_SIZE];
static CPU_STK s_aht_stk[TASK_AHT_STK_SIZE];

/* ------------------------------------------------------------------ 辅助 */
#if (APP_TASK3_AHT20 != 0)
/* 把浮点数格式化成 "xx.xx"，避免 -5.-24 这种负号进位错误 */
static void FmtFix2(char *dst, size_t size, float v)
{
    int32_t  x = (int32_t)((v >= 0.0f) ? (v * 100.0f + 0.5f) : (v * 100.0f - 0.5f));
    uint32_t a = (uint32_t)((x < 0) ? -x : x);

    (void)snprintf(dst, size, "%s%lu.%02lu",
                   (x < 0) ? "-" : "",
                   (unsigned long)(a / 100U),
                   (unsigned long)(a % 100U));
}
#endif

/* active_level = 1 高电平点亮，= 0 低电平点亮 */
static void Led_Write(GPIO_TypeDef *port, uint16_t pin, uint8_t active_level, uint8_t on)
{
    GPIO_PinState state;

    if (active_level != 0U)
    {
        state = (on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }
    else
    {
        state = (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    }

    HAL_GPIO_WritePin(port, pin, state);
}

#if (APP_LOG_LED != 0) || (APP_TASK3_AHT20 != 0)
/* 当前内核时间（tick = 1 ms） */
static uint32_t NowMs(void)
{
    OS_ERR err;

    return (uint32_t)OSTimeGet(&err);
}
#endif

/* ------------------------------------------------------- 任务1：LED1 1s */
static void Task_Led1(void *p_arg)
{
    OS_ERR  err;
    uint8_t on = 0U;

    (void)p_arg;

#if (APP_DIAG_BOOT != 0)
    BSP_UART_Printf("[CHK] TASK1 entered (prio %u)\r\n", (unsigned)TASK_LED1_PRIO);
#endif
    for (;;)
    {
        on = (on != 0U) ? 0U : 1U;
        Led_Write(BOARD_LED1_PORT, BOARD_LED1_PIN, BOARD_LED1_ON_LEVEL, on);

#if (APP_LOG_LED != 0)
        BSP_UART_Printf("[%8lu ms] TASK1  LED1 -> %s\r\n",
                        (unsigned long)NowMs(),
                        (on != 0U) ? "ON " : "OFF");
#endif
        OSTimeDlyHMSM(0U, 0U, 1U, 0U, OS_OPT_TIME_HMSM_STRICT, &err);   /* 睡 1 s */
    }
}

/* ------------------------------------------------------- 任务2：LED2 3s */
static void Task_Led2(void *p_arg)
{
    OS_ERR  err;
    uint8_t on = 0U;

    (void)p_arg;

#if (APP_DIAG_BOOT != 0)
    BSP_UART_Printf("[CHK] TASK2 entered (prio %u)\r\n", (unsigned)TASK_LED2_PRIO);
#endif
    for (;;)
    {
        on = (on != 0U) ? 0U : 1U;
        Led_Write(BOARD_LED2_PORT, BOARD_LED2_PIN, BOARD_LED2_ON_LEVEL, on);

#if (APP_LOG_LED != 0)
        BSP_UART_Printf("[%8lu ms] TASK2  LED2 -> %s\r\n",
                        (unsigned long)NowMs(),
                        (on != 0U) ? "ON " : "OFF");
#endif
        OSTimeDlyHMSM(0U, 0U, 3U, 0U, OS_OPT_TIME_HMSM_STRICT, &err);   /* 睡 3 s */
    }
}

/* ------------------------------------------- 任务3：hello + AHT20（2s）
 *
 * 这里可以放心地写"阻塞式"采集（AHT20_ReadBlocking 内部 OSTimeDly(80)），
 * 因为 uC/OS-III 是抢占式内核：任务睡觉时 CPU 立刻去跑 LED 任务。
 * 这正是用 RTOS 相比裸机 HAL_Delay 的最大区别。
 */
static void Task_Aht20(void *p_arg)
{
    OS_ERR   err;
    float    temp_c  = 0.0f;
    float    humi_rh = 0.0f;
    char     t_buf[12];
    char     h_buf[12];
    uint8_t  ret;

    (void)p_arg;

#if (APP_DIAG_BOOT != 0)
    BSP_UART_Printf("[CHK] TASK3 entered (prio %u)\r\n", (unsigned)TASK_AHT_PRIO);
#endif
    for (;;)
    {
        /* ---- 任务一的要求：每 2 s 打印一次 ---- */
        BSP_UART_Printf("hello uc/OS! 欢迎来到RTOS多任务环境！\r\n");

#if (APP_TASK3_AHT20 != 0)
        /* ---- 任务二第 3 小问：每 2 s 采一次温湿度 ---- */
        ret = AHT20_ReadBlocking(&temp_c, &humi_rh);

        if (ret == AHT20_OK)
        {
            FmtFix2(t_buf, sizeof(t_buf), temp_c);
            FmtFix2(h_buf, sizeof(h_buf), humi_rh);
            BSP_UART_Printf("[%8lu ms] TASK3  T=%s C  H=%s %%RH  (CRC %s)\r\n",
                            (unsigned long)NowMs(), t_buf, h_buf,
                            (AHT20_LastCrcOk() != 0U) ? "ok" : "NG");
        }
        else
        {
            BSP_UART_Printf("[%8lu ms] TASK3  AHT20 read FAIL (ret=%u)\r\n",
                            (unsigned long)NowMs(), (unsigned)ret);
        }
#else
        (void)temp_c; (void)humi_rh; (void)t_buf; (void)h_buf; (void)ret;
#endif

        OSTimeDlyHMSM(0U, 0U, 2U, 0U, OS_OPT_TIME_HMSM_STRICT, &err);   /* 睡 2 s */
    }
}

/* ------------------------------------------------------------ 初始化入口 */
void App_TasksInit(void)
{
    OS_ERR     err;
    CPU_INT16U ver;

    ver = OSVersion(&err);

    BSP_UART_Printf("\r\n");
    BSP_UART_Printf("==================================================\r\n");
    BSP_UART_Printf("  STM32F103C8T6 + AHT20  uC/OS-III 多任务实验\r\n");
    BSP_UART_Printf("  姓名: %s   学号: %s\r\n", APP_STUDENT_NAME, APP_STUDENT_ID);
    BSP_UART_Printf("  RTOS : uC/OS-III V%u.%02u.%02u, tick %u Hz\r\n",
                    (unsigned)(ver / 10000u),
                    (unsigned)((ver / 100u) % 100u),
                    (unsigned)(ver % 100u),
                    (unsigned)OS_CFG_TICK_RATE_HZ);
    BSP_UART_Printf("  CPU  : %lu MHz (HSI), PendSV/SysTick 已交给内核\r\n",
                    (unsigned long)(SystemCoreClock / 1000000U));
    BSP_UART_Printf("  TASK1: LED1 每 %lu ms 翻转 (prio %u)\r\n",
                    (unsigned long)APP_LED1_PERIOD_MS, (unsigned)TASK_LED1_PRIO);
    BSP_UART_Printf("  TASK2: LED2 每 %lu ms 翻转 (prio %u)\r\n",
                    (unsigned long)APP_LED2_PERIOD_MS, (unsigned)TASK_LED2_PRIO);
#if (APP_TASK3_AHT20 != 0)
    BSP_UART_Printf("  TASK3: hello + AHT20 每 %lu ms 一次 (prio %u)\r\n",
                    (unsigned long)APP_AHT20_PERIOD_MS, (unsigned)TASK_AHT_PRIO);
    BSP_UART_Printf("  I2C  : %s @ PB6/PB7, AHT20 addr 0x%02X\r\n",
                    (APP_I2C_USE_SOFT != 0) ? "软件模拟 (GPIO)" : "硬件外设 I2C1",
                    (unsigned)AHT20_I2C_ADDR7);
#else
    BSP_UART_Printf("  TASK3: hello uc/OS! 每 %lu ms 一次 (prio %u)\r\n",
                    (unsigned long)APP_HELLO_PERIOD_MS, (unsigned)TASK_AHT_PRIO);
#endif
    BSP_UART_Printf("==================================================\r\n");

#if (APP_TASK3_AHT20 != 0)
    /* 上电先初始化一次（调度器还没跑，AHT20_Init 内部走 HAL_Delay） */
    if (AHT20_Init() == AHT20_OK)
    {
        BSP_UART_Printf("[%8lu ms] BOOT   AHT20 init OK\r\n", (unsigned long)HAL_GetTick());
    }
    else
    {
        BSP_UART_Printf("[%8lu ms] BOOT   AHT20 init FAIL, task3 will retry\r\n",
                        (unsigned long)HAL_GetTick());
    }
#endif

    /* ---- 创建三个任务 ---- */
    OSTaskCreate(&s_led1_tcb, "LED1", Task_Led1, (void *)0,
                 TASK_LED1_PRIO, &s_led1_stk[0], TASK_LED1_STK_SIZE / 10u,
                 TASK_LED1_STK_SIZE, 0u, 0u, (void *)0, 0u, &err);
    if (err != OS_ERR_NONE)     /* 创建失败最常见的原因：优先级冲突 / 栈太小 / 重复创建 */
    {
        BSP_UART_Printf("[BOOT] OSTaskCreate(LED1) FAIL, err=%u\r\n", (unsigned)err);
        DIAG_Panic(6U);         /* PC13 闪 6 下 */
    }

    OSTaskCreate(&s_led2_tcb, "LED2", Task_Led2, (void *)0,
                 TASK_LED2_PRIO, &s_led2_stk[0], TASK_LED2_STK_SIZE / 10u,
                 TASK_LED2_STK_SIZE, 0u, 0u, (void *)0, 0u, &err);
    if (err != OS_ERR_NONE)     /* 创建失败最常见的原因：优先级冲突 / 栈太小 / 重复创建 */
    {
        BSP_UART_Printf("[BOOT] OSTaskCreate(LED2) FAIL, err=%u\r\n", (unsigned)err);
        DIAG_Panic(6U);         /* PC13 闪 6 下 */
    }

    OSTaskCreate(&s_aht_tcb, "TASK3", Task_Aht20, (void *)0,
                 TASK_AHT_PRIO, &s_aht_stk[0], TASK_AHT_STK_SIZE / 10u,
                 TASK_AHT_STK_SIZE, 0u, 0u, (void *)0, 0u, &err);
    if (err != OS_ERR_NONE)     /* 创建失败最常见的原因：优先级冲突 / 栈太小 / 重复创建 */
    {
        BSP_UART_Printf("[BOOT] OSTaskCreate(TASK3) FAIL, err=%u\r\n", (unsigned)err);
        DIAG_Panic(6U);         /* PC13 闪 6 下 */
    }

    BSP_UART_Printf("[%8lu ms] OS     tasks created, OSStart() ...\r\n",
                    (unsigned long)HAL_GetTick());
}
