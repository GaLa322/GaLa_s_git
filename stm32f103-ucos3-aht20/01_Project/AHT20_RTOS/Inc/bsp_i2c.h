/**
  ******************************************************************************
  * @file    bsp_i2c.h
  * @brief   I2C 总线的统一接口。底层可以是硬件 I2C1，也可以是软件 GPIO 模拟，
  *          由 board_config.h 里的 APP_I2C_USE_SOFT 决定，上层 AHT20 驱动不用改。
  ******************************************************************************
  */
#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 返回 1 成功，0 失败（无应答、超时、总线错误都算失败） */
uint8_t BSP_I2C_Init(void);
uint8_t BSP_I2C_Write(uint8_t addr7, const uint8_t *data, uint16_t len);
uint8_t BSP_I2C_Read(uint8_t addr7, uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_I2C_H */
