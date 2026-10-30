/**
  ******************************************************************************
  * @file    diag.h
  * @brief   上电自检 / 死机指示：不依赖 HAL、不依赖 uC/OS 的最小诊断工具。
  *
  *  为什么要它？
  *    移植内核后如果程序在 main 前半段就挂了（Error_Handler、HardFault、
  *    OSStart 前卡死），现象是"灯不亮、串口也不打印"——什么线索都没有。
  *    这个模块只用寄存器直接点 PC13，先闪 3 下证明"CPU 活着"，
  *    死机后用不同的闪烁次数告诉你"死在哪一类错误"。
  ******************************************************************************
  */
#ifndef __DIAG_H
#define __DIAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 把 PC13 配成推挽输出（直接写寄存器，任何时刻调用都安全） */
void DIAG_PinInit(void);

/* 闪 n 下（on/off 单位 ms）。用于"我还活着"的心跳 */
void DIAG_Blink(uint32_t times, uint32_t on_ms, uint32_t off_ms);

/**
  * @brief 死机指示：先闪 code 下，停顿一下再重复，永不返回。
  *        code = 1 : Error_Handler（HAL 初始化失败 / 外设初始化失败）
  *        code = 2 : HardFault
  *        code = 3 : MemManage
  *        code = 4 : BusFault
  *        code = 5 : UsageFault
  *        code = 6 : OSTaskCreate 失败（闪 6 下，看串口里的错误码）
  */
void DIAG_Panic(uint32_t code);

/**
  * @brief 裸机自检：完全绕开 uC/OS 与 HAL 延时，只做「闪灯 + 串口发字符串」。
  *        用来把问题劈成两半：这段能跑 -> 板子/时钟/串口没问题，问题在内核；
  *        这段也不跑 -> 板子、下载、Boot0、复位这些硬件层面的事。
  *        永不返回（由 board_config.h 的 APP_DIAG_BARE 打开）。
  */
void DIAG_BareLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* __DIAG_H */
