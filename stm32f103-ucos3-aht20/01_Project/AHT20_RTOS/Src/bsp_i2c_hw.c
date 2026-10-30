/**
  ******************************************************************************
  * @file    bsp_i2c_hw.c
  * @brief   硬件 I2C1（PB6=SCL, PB7=SDA, 400kHz）实现。
  *          只有 APP_I2C_USE_SOFT == 0 时参与编译。
  ******************************************************************************
  */
#include "bsp_i2c.h"
#include "board_config.h"
#include "i2c.h"

#if (APP_I2C_USE_SOFT == 0)

uint8_t BSP_I2C_Init(void)
{
    return 1U;   /* CubeMX 的 MX_I2C1_Init() 已经在 main 里调用过了 */
}

uint8_t BSP_I2C_Write(uint8_t addr7, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit(&hi2c1,
                                  (uint16_t)((uint16_t)addr7 << 1),
                                  (uint8_t *)data, len, APP_I2C_TIMEOUT_MS);

    return (ret == HAL_OK) ? 1U : 0U;
}

uint8_t BSP_I2C_Read(uint8_t addr7, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Receive(&hi2c1,
                                 (uint16_t)((uint16_t)addr7 << 1),
                                 data, len, APP_I2C_TIMEOUT_MS);

    return (ret == HAL_OK) ? 1U : 0U;
}

#endif /* APP_I2C_USE_SOFT == 0 */
