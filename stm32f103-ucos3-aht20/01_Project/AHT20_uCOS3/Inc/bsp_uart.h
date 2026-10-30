/**
  ******************************************************************************
  * @file    bsp_uart.h
  * @brief   USART1（PA9=TX, PA10=RX, 115200 8N1）的打印接口。
  *
  *          不用 printf：Keil AC6 下 printf 会引半主机（semihosting），
  *          一调用就停住。这里用 vsnprintf 拼好字符串再整块发给串口，最稳。
  ******************************************************************************
  */
#ifndef __BSP_UART_H
#define __BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void BSP_UART_Write(const uint8_t *data, uint16_t len);
void BSP_UART_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H */
