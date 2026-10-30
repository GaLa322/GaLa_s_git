/**
  ******************************************************************************
  * @file    app_tasks.c
  * @brief   三个任务的实现（任务体与将来 uC/OS-III 版本完全一致）
  *
  *  任务         周期      作用
  *  ----------   --------  ---------------------------------------------
  *  Task_Led1    1000 ms   翻转 LED1（PC13，板载），1s 亮 / 1s 灭
  *  Task_Led2    3000 ms   翻转 LED2（PB12，外接），3s 亮 / 3s 灭
  *  Task_Aht20   10 ms     轮询状态机：每 2s 采一次温湿度并串口打印
  ******************************************************************************
  */
#include "app_tasks.h"
#include "board_config.h"
#include "osal.h"
#include "aht20.h"
#include "bsp_uart.h"
#include "main.h"
#include <stdio.h>

/* ------------------------------------------------------------------ 辅助 */
/* 把浮点数格式化成 "xx.xx"，避免在串口里出现 -5.-24 这种负号进位错误 */
static void FmtFix2(char *dst, size_t size, float v)
{
    int32_t  x = (int32_t)((v >= 0.0f) ? (v * 100.0f + 0.5f) : (v * 100.0f - 0.5f));
    uint32_t a = (uint32_t)((x < 0) ? -x : x);

    (void)snprintf(dst, size, "%s%lu.%02lu",
                   (x < 0) ? "-" : "",
                   (unsigned long)(a / 100U),
                   (unsigned long)(a % 100U));
}

/* active_level = 1 表示高电平点亮，= 0 表示低电平点亮 */
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

/* ------------------------------------------------------- 任务1：LED1 1s */
static uint8_t s_led1_on = 0U;

static void Task_Led1(void *arg)
{
    (void)arg;

    s_led1_on = (s_led1_on != 0U) ? 0U : 1U;
    Led_Write(BOARD_LED1_PORT, BOARD_LED1_PIN, BOARD_LED1_ON_LEVEL, s_led1_on);

#if (APP_LOG_LED != 0)
    BSP_UART_Printf("[%8lu ms] TASK1  LED1 -> %s\r\n",
                    (unsigned long)HAL_GetTick(),
                    (s_led1_on != 0U) ? "ON " : "OFF");
#endif
}

/* ------------------------------------------------------- 任务2：LED2 3s */
static uint8_t s_led2_on = 0U;

static void Task_Led2(void *arg)
{
    (void)arg;

    s_led2_on = (s_led2_on != 0U) ? 0U : 1U;
    Led_Write(BOARD_LED2_PORT, BOARD_LED2_PIN, BOARD_LED2_ON_LEVEL, s_led2_on);

#if (APP_LOG_LED != 0)
    BSP_UART_Printf("[%8lu ms] TASK2  LED2 -> %s\r\n",
                    (unsigned long)HAL_GetTick(),
                    (s_led2_on != 0U) ? "ON " : "OFF");
#endif
}

/* ------------------------------------------- 任务3：AHT20 采集（每 2s）
 *
 * 为什么写成状态机，而不是像裸机那样 HAL_Delay(80)？
 *   本工程的 OSAL 是"协作式"调度：一个任务不主动让出 CPU，别的任务就得等。
 *   如果这里 HAL_Delay(80)，LED 的 1s/3s 节拍会被拖成 1.08s/3.08s，而且
 *   串口打印会抖。所以把"等 80ms 转换"拆成 WAIT 状态，让任务每 10ms
 *   回来看一眼，CPU 在等待期间完全可以让给别的任务。
 *   换成真正的 uC/OS 后，就可以直接写 AHT20_ReadBlocking()，
 *   因为内核会在阻塞时自动切换任务 —— 这正是"为什么要用 RTOS"的直观例子。
 */
typedef enum
{
    AHT_ST_INIT = 0,
    AHT_ST_IDLE,
    AHT_ST_WAIT
} AhtState;

static AhtState  s_state   = AHT_ST_INIT;
static uint32_t  s_t0      = 0U;      /* 触发转换的时刻 */
static uint32_t  s_next_ms = 0U;      /* 下一次动作的时刻 */
static uint32_t  s_ok_cnt  = 0U;
static uint32_t  s_err_cnt = 0U;

static void Aht_PrintError(uint32_t now, const char *what)
{
    s_err_cnt++;
    BSP_UART_Printf("[%8lu ms] TASK3  AHT20 %s FAIL (err=%lu)\r\n",
                    (unsigned long)now, what, (unsigned long)s_err_cnt);
}

static void Task_Aht20(void *arg)
{
    float    temp_c = 0.0f;
    float    humi_rh = 0.0f;
    uint8_t  ret;
    uint32_t now = HAL_GetTick();

    (void)arg;

    switch (s_state)
    {
    case AHT_ST_INIT:
        if ((int32_t)(now - s_next_ms) < 0)
        {
            break;                                  /* 还没到重试时间 */
        }
        if (AHT20_Init() == AHT20_OK)
        {
            BSP_UART_Printf("[%8lu ms] TASK3  AHT20 init OK (addr 0x%02X)\r\n",
                            (unsigned long)now, (unsigned)AHT20_I2C_ADDR7);
            s_state   = AHT_ST_IDLE;
            s_next_ms = now;                        /* 立刻开始第一次采集 */
        }
        else
        {
            Aht_PrintError(now, "init");
            s_next_ms = now + 1000U;                /* 1s 后重试 */
        }
        break;

    case AHT_ST_IDLE:
        if ((int32_t)(now - s_next_ms) < 0)
        {
            break;                                  /* 还没到 2s 采样点 */
        }
        if (AHT20_StartMeasurement() != AHT20_OK)
        {
            Aht_PrintError(now, "trigger");
            s_next_ms = now + 500U;
            break;
        }
        s_t0    = now;
        s_state = AHT_ST_WAIT;
        break;

    case AHT_ST_WAIT:
        if ((uint32_t)(now - s_t0) < AHT20_MEASURE_MS)
        {
            break;                                  /* 80ms 还没到，下轮再看 */
        }
        ret = AHT20_ReadMeasurement(&temp_c, &humi_rh);

        if (ret == AHT20_BUSY)
        {
            break;                                  /* 芯片还在转换，10ms 后再读 */
        }
        if (ret == AHT20_OK)
        {
            char t_buf[12];
            char h_buf[12];

            s_ok_cnt++;
            FmtFix2(t_buf, sizeof(t_buf), temp_c);
            FmtFix2(h_buf, sizeof(h_buf), humi_rh);

            /* 任务二(2)：串口只输出温湿度，不带时间戳/CRC，形如
             *     T=23.82 C  H=62.76 %RH                                  */
            (void)AHT20_LastCrcOk();                /* CRC 只用于内部判断，不打印 */
            BSP_UART_Printf("T=%s C  H=%s %%RH\r\n", t_buf, h_buf);
        }
        else
        {
            Aht_PrintError(now, "read");
        }

        s_state   = AHT_ST_IDLE;
        s_next_ms = now + APP_AHT20_PERIOD_MS;      /* 固定节拍，不累积漂移 */
        break;

    default:
        s_state = AHT_ST_INIT;
        break;
    }
}

/* ------------------------------------------------------------ 初始化入口 */
void App_TasksInit(void)
{
    OSAL_Init();

    BSP_UART_Printf("\r\n");
    BSP_UART_Printf("==================================================\r\n");
    BSP_UART_Printf("  STM32F103C8T6 + AHT20  RTOS 多任务实验\r\n");
    BSP_UART_Printf("  姓名: %s   学号: %s\r\n", APP_STUDENT_NAME, APP_STUDENT_ID);
    BSP_UART_Printf("  CPU : %lu MHz (HSI)\r\n", (unsigned long)(SystemCoreClock / 1000000U));
    BSP_UART_Printf("  TASK1: LED1 每 %lu ms 翻转一次\r\n", (unsigned long)APP_LED1_PERIOD_MS);
    BSP_UART_Printf("  TASK2: LED2 每 %lu ms 翻转一次\r\n", (unsigned long)APP_LED2_PERIOD_MS);
    BSP_UART_Printf("  TASK3: AHT20 每 %lu ms 采集一次\r\n", (unsigned long)APP_AHT20_PERIOD_MS);
    BSP_UART_Printf("  I2C  : %s @ PB6/PB7, AHT20 addr 0x%02X\r\n",
                    (APP_I2C_USE_SOFT != 0) ? "软件模拟 (GPIO)" : "硬件外设 I2C1",
                    (unsigned)AHT20_I2C_ADDR7);
    BSP_UART_Printf("==================================================\r\n");

    /* 上电先试一次；失败也不要紧，Task3 会在 1s 后自动重试 */
    if (AHT20_Init() == AHT20_OK)
    {
        BSP_UART_Printf("[%8lu ms] BOOT   AHT20 init OK\r\n", (unsigned long)HAL_GetTick());
        s_state   = AHT_ST_IDLE;
        s_next_ms = HAL_GetTick();
    }
    else
    {
        BSP_UART_Printf("[%8lu ms] BOOT   AHT20 init FAIL, task3 will retry\r\n",
                        (unsigned long)HAL_GetTick());
        s_state   = AHT_ST_INIT;
        s_next_ms = HAL_GetTick() + 1000U;
    }

    (void)OSAL_TaskCreate("LED1",  Task_Led1,   0, APP_LED1_PERIOD_MS);
    (void)OSAL_TaskCreate("LED2",  Task_Led2,   0, APP_LED2_PERIOD_MS);
    (void)OSAL_TaskCreate("AHT20", Task_Aht20,  0, APP_AHT20_POLL_MS);

    BSP_UART_Printf("[%8lu ms] OSAL   %lu tasks created, starting...\r\n",
                    (unsigned long)HAL_GetTick(), (unsigned long)OSAL_TaskCount());
}
