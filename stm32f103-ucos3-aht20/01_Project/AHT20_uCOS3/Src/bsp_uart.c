/**
  ******************************************************************************
  * @file    bsp_uart.c
  * @brief   串口打印实现（USART1，115200 8N1）
  ******************************************************************************
  */
#include "bsp_uart.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>

void BSP_UART_Write(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U))
    {
        return;
    }
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}

void BSP_UART_Printf(const char *fmt, ...)
{
    char    buf[160];
    int     n;
    va_list ap;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n <= 0)
    {
        return;
    }
    if (n > (int)(sizeof(buf) - 1U))
    {
        n = (int)(sizeof(buf) - 1U);   /* 被截断了，只发能发出去的部分 */
    }

    BSP_UART_Write((const uint8_t *)buf, (uint16_t)n);
}
