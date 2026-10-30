/**
  ******************************************************************************
  * @file    aht20.c
  * @brief   AHT20 驱动实现
  ******************************************************************************
  */
#include "aht20.h"
#include "board_config.h"
#include "bsp_i2c.h"
#include "main.h"

#if (APP_USE_UCOS3 != 0)
#include "os.h"
/* 调度器在跑 -> OSTimeDly（让出 CPU）；还没启动 -> HAL_Delay（忙等） */
static void AHT_Delay(uint32_t ms)
{
    if (OSRunning == OS_STATE_OS_RUNNING)
    {
        OS_ERR err;
        OSTimeDly(ms, OS_OPT_TIME_DLY, &err);
    }
    else
    {
        HAL_Delay(ms);
    }
}
#else
#define AHT_Delay(ms)   HAL_Delay(ms)
#endif

#define AHT20_CMD_INIT          0xBEU   /* 初始化/校准命令 */
#define AHT20_CMD_TRIGGER       0xACU   /* 触发测量命令   */
#define AHT20_CMD_SOFT_RESET    0xBAU   /* 软复位（本工程暂未使用） */

#define AHT20_STATUS_BUSY       0x80U   /* bit7：1 = 忙，数据无效 */
#define AHT20_STATUS_CAL        0x08U   /* bit3：1 = 已校准   */

static uint8_t s_crc_ok = 1U;

/* AHT20 的 CRC8：多项式 0x31，初值 0xFF，覆盖前 6 个字节 */
static uint8_t AHT20_Crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFFU;
    uint8_t i;
    uint8_t bit;

    for (i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 0x80U) != 0U) ? (uint8_t)((crc << 1) ^ 0x31U)
                                        : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

uint8_t AHT20_Init(void)
{
    uint8_t status = 0U;
    uint8_t cmd[3];
    uint8_t i;

    AHT_Delay(100U);            /* 上电后器件需要稳定时间，手册要求 > 40ms */

    if (BSP_I2C_Read(AHT20_I2C_ADDR7, &status, 1U) == 0U)
    {
        return AHT20_ERROR;     /* 没应答：查供电、SDA/SCL、上拉、地址 */
    }

    if ((status & AHT20_STATUS_CAL) == 0U)
    {
        cmd[0] = AHT20_CMD_INIT;
        cmd[1] = 0x08U;
        cmd[2] = 0x00U;
        if (BSP_I2C_Write(AHT20_I2C_ADDR7, cmd, 3U) == 0U)
        {
            return AHT20_ERROR;
        }
        AHT_Delay(10U);
    }

    /* 回读状态，确认 CAL 位真的置起来了 */
    for (i = 0U; i < 5U; i++)
    {
        if (BSP_I2C_Read(AHT20_I2C_ADDR7, &status, 1U) == 0U)
        {
            return AHT20_ERROR;
        }
        if ((status & AHT20_STATUS_CAL) != 0U)
        {
            return AHT20_OK;
        }
        AHT_Delay(10U);
    }

    return AHT20_ERROR;
}

uint8_t AHT20_StartMeasurement(void)
{
    uint8_t cmd[3];

    cmd[0] = AHT20_CMD_TRIGGER;
    cmd[1] = 0x33U;
    cmd[2] = 0x00U;

    return BSP_I2C_Write(AHT20_I2C_ADDR7, cmd, 3U);
}

uint8_t AHT20_ReadMeasurement(float *temperature, float *humidity)
{
    uint8_t  buf[7];
    uint32_t raw_humi;
    uint32_t raw_temp;

    if ((temperature == 0) || (humidity == 0))
    {
        return AHT20_ERROR;
    }

    if (BSP_I2C_Read(AHT20_I2C_ADDR7, buf, 7U) == 0U)
    {
        return AHT20_ERROR;
    }

    if ((buf[0] & AHT20_STATUS_BUSY) != 0U)
    {
        return AHT20_BUSY;      /* 80ms 还没到，等会儿再来 */
    }

    s_crc_ok = (AHT20_Crc8(buf, 6U) == buf[6]) ? 1U : 0U;

    /* 湿度 20bit = buf[1] + buf[2] + buf[3] 高 4 位 */
    raw_humi = ((uint32_t)buf[1] << 12) |
               ((uint32_t)buf[2] << 4)  |
               ((uint32_t)buf[3] >> 4);

    /* 温度 20bit = buf[3] 低 4 位 + buf[4] + buf[5]，buf[6] 是 CRC 不能算进来 */
    raw_temp = (((uint32_t)buf[3] & 0x0FU) << 16) |
               ((uint32_t)buf[4] << 8) |
               ((uint32_t)buf[5]);

    *humidity    = ((float)raw_humi * 100.0f) / 1048576.0f;
    *temperature = ((float)raw_temp * 200.0f) / 1048576.0f - 50.0f;

    return AHT20_OK;
}

uint8_t AHT20_ReadBlocking(float *temperature, float *humidity)
{
    uint8_t i;
    uint8_t ret;

    if (AHT20_StartMeasurement() == 0U)
    {
        return AHT20_ERROR;
    }

    AHT_Delay(AHT20_MEASURE_MS);

    for (i = 0U; i < 5U; i++)
    {
        ret = AHT20_ReadMeasurement(temperature, humidity);

        if (ret == AHT20_OK)
        {
            return AHT20_OK;
        }
        if (ret == AHT20_ERROR)
        {
            return AHT20_ERROR;
        }
        AHT_Delay(10U);
    }

    return AHT20_BUSY;
}

uint8_t AHT20_LastCrcOk(void)
{
    return s_crc_ok;
}
