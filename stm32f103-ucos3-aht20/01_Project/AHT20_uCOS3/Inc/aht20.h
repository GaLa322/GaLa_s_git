/**
  ******************************************************************************
  * @file    aht20.h
  * @brief   AHT20 温湿度传感器驱动（I2C，7 位地址 0x38）
  *
  *          读一次数据的完整流程（数据手册 5.2 节）：
  *            1. 上电 -> 等 >= 40 ms；
  *            2. 读状态字，bit3(CAL)=0 说明没校准 -> 发 0xBE 0x08 0x00；
  *            3. 发触发测量命令 0xAC 0x33 0x00；
  *            4. 等 80 ms，读 7 个字节；
  *            5. 状态字 bit7=1 表示还在忙，数据无效；
  *            6. 湿度 = 20bit / 2^20 * 100%，温度 = 20bit / 2^20 * 200 - 50。
  ******************************************************************************
  */
#ifndef __AHT20_H
#define __AHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define AHT20_OK            1U   /* 成功 */
#define AHT20_BUSY          2U   /* 器件还在测量 */
#define AHT20_ERROR         0U   /* I2C 无应答 / 读失败 */

#define AHT20_MEASURE_MS    80U  /* 手册：触发后典型 80 ms 出结果 */

/* 初始化：上电延时、读状态、必要时写校准命令，并回读确认 CAL=1 */
uint8_t AHT20_Init(void);

/* 只发触发命令，立刻返回（非阻塞），配合 AHT20_ReadMeasurement 用 */
uint8_t AHT20_StartMeasurement(void);

/* 读 7 字节并换算。返回 AHT20_BUSY 时数据无效，等 10ms 再读一次 */
uint8_t AHT20_ReadMeasurement(float *temperature, float *humidity);

/* 阻塞版：触发 + 等 80ms + 读，一条龙。适合放到 uC/OS 任务里用 */
uint8_t AHT20_ReadBlocking(float *temperature, float *humidity);

/* 最近一次读到的 CRC 是否匹配（1=匹配）。仅作诊断，不影响数据 */
uint8_t AHT20_LastCrcOk(void);

#ifdef __cplusplus
}
#endif

#endif /* __AHT20_H */
