/**
  ******************************************************************************
  * @file    bsp_i2c_sw.c
  * @brief   软件 I2C（GPIO 模拟，PB6=SCL, PB7=SDA，约 100kHz）。
  *          只有 APP_I2C_USE_SOFT == 1 时参与编译。
  *
  *          和硬件 I2C 的差别就体现在这里：
  *          硬件 I2C 由外设产生 START/STOP/9 个时钟，CPU 只管读写 DR；
  *          软件 I2C 则是"人肉"把 SCL/SDA 按时序拉高拉低，CPU 全程陪着。
  ******************************************************************************
  */
#include "bsp_i2c.h"
#include "board_config.h"
#include "main.h"

#if (APP_I2C_USE_SOFT == 1)

#define I2C_GPIO_PORT       GPIOB
#define I2C_SCL_PIN         GPIO_PIN_6
#define I2C_SDA_PIN         GPIO_PIN_7

#define SCL_HIGH()          HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET)
#define SCL_LOW()           HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_RESET)
#define SDA_HIGH()          HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_SET)
#define SDA_LOW()           HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_RESET)
#define SDA_READ()          HAL_GPIO_ReadPin(I2C_GPIO_PORT, I2C_SDA_PIN)

static uint32_t s_cycles_per_us = 8U;

static void I2C_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * s_cycles_per_us;

    while ((DWT->CYCCNT - start) < ticks)
    {
        /* busy wait */
    }
}

static void I2C_Start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    I2C_DelayUs(5U);
    SDA_LOW();          /* SCL 高时 SDA 下降 = START */
    I2C_DelayUs(5U);
    SCL_LOW();
    I2C_DelayUs(5U);
}

static void I2C_Stop(void)
{
    SDA_LOW();
    I2C_DelayUs(5U);
    SCL_HIGH();
    I2C_DelayUs(5U);
    SDA_HIGH();         /* SCL 高时 SDA 上升 = STOP */
    I2C_DelayUs(5U);
}

/* 返回 1 = 收到 ACK，0 = NACK */
static uint8_t I2C_WriteByte(uint8_t byte)
{
    uint8_t i;
    uint8_t ack;

    for (i = 0U; i < 8U; i++)
    {
        if ((byte & 0x80U) != 0U)
        {
            SDA_HIGH();
        }
        else
        {
            SDA_LOW();
        }
        byte <<= 1;

        I2C_DelayUs(2U);
        SCL_HIGH();
        I2C_DelayUs(5U);
        SCL_LOW();
        I2C_DelayUs(2U);
    }

    SDA_HIGH();          /* 释放 SDA，等从机拉低应答 */
    I2C_DelayUs(2U);
    SCL_HIGH();
    I2C_DelayUs(2U);
    ack = (SDA_READ() == GPIO_PIN_RESET) ? 1U : 0U;
    I2C_DelayUs(3U);
    SCL_LOW();
    I2C_DelayUs(2U);

    return ack;
}

static uint8_t I2C_ReadByte(uint8_t send_ack)
{
    uint8_t i;
    uint8_t byte = 0U;

    SDA_HIGH();          /* 主机释放 SDA，交给从机驱动 */

    for (i = 0U; i < 8U; i++)
    {
        I2C_DelayUs(2U);
        SCL_HIGH();
        I2C_DelayUs(3U);
        byte <<= 1;
        if (SDA_READ() == GPIO_PIN_SET)
        {
            byte |= 1U;
        }
        I2C_DelayUs(2U);
        SCL_LOW();
        I2C_DelayUs(3U);
    }

    if (send_ack != 0U)
    {
        SDA_LOW();       /* ACK：把 SDA 拉低 */
    }
    else
    {
        SDA_HIGH();      /* NACK：最后一个字节 */
    }
    I2C_DelayUs(2U);
    SCL_HIGH();
    I2C_DelayUs(3U);
    SCL_LOW();
    I2C_DelayUs(2U);
    SDA_HIGH();

    return byte;
}

/* 总线卡死时（SDA 被从机拉低）打 9 个时钟把它救回来 */
static void I2C_BusRecover(void)
{
    uint8_t i;

    SDA_HIGH();
    for (i = 0U; i < 9U; i++)
    {
        SCL_LOW();
        I2C_DelayUs(5U);
        SCL_HIGH();
        I2C_DelayUs(5U);
    }
    I2C_Stop();
}

uint8_t BSP_I2C_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN | I2C_SDA_PIN, GPIO_PIN_SET);

    /* 开漏输出：和真正的 I2C 一样，只能拉低，高电平靠上拉电阻 */
    gpio.Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_GPIO_PORT, &gpio);

    /* 用 DWT 周期计数器做微秒级延时，比 for 循环准 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    s_cycles_per_us = SystemCoreClock / 1000000U;
    if (s_cycles_per_us == 0U)
    {
        s_cycles_per_us = 8U;
    }

    I2C_BusRecover();
    return 1U;
}

uint8_t BSP_I2C_Write(uint8_t addr7, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t  ok = 1U;

    I2C_Start();

    if (I2C_WriteByte((uint8_t)((uint8_t)addr7 << 1)) == 0U)
    {
        ok = 0U;
    }

    for (i = 0U; (i < len) && (ok != 0U); i++)
    {
        if (I2C_WriteByte(data[i]) == 0U)
        {
            ok = 0U;
        }
    }

    I2C_Stop();
    return ok;
}

uint8_t BSP_I2C_Read(uint8_t addr7, uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t  ok = 1U;

    I2C_Start();

    if (I2C_WriteByte((uint8_t)(((uint8_t)addr7 << 1) | 0x01U)) == 0U)
    {
        ok = 0U;
    }

    for (i = 0U; (i < len) && (ok != 0U); i++)
    {
        data[i] = I2C_ReadByte((i < (uint16_t)(len - 1U)) ? 1U : 0U);
    }

    I2C_Stop();
    return ok;
}

#endif /* APP_I2C_USE_SOFT == 1 */
