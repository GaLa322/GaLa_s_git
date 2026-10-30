/**
  ******************************************************************************
  * @file    diag.c
  * @brief   上电自检 / 死机指示（只用寄存器，不依赖 HAL 与 uC/OS 的任何状态）
  *
  *  设计原则：
  *    1) 任何时刻调用都不能把已经跑好的系统搞坏 -> 只动 GPIOC 的 PC13；
  *    2) 不能依赖 HAL_Delay（SysTick 可能被内核接管或已停）-> 用空循环；
  *    3) 不能依赖串口（串口本身就是怀疑对象）-> 灯是唯一的"输出设备"。
  ******************************************************************************
  */
#include "diag.h"
#include "main.h"

/* PC13 = Blue Pill 板载 LED（低电平点亮）。这里只做"翻转"，所以不用管极性。 */
static void diag_toggle(void)
{
    GPIOC->ODR ^= GPIO_ODR_ODR13;
}

/* 粗延时：8 MHz 下大约 1 ms（主频变了只是快慢不同，闪灯照样看得见） */
static void diag_delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    volatile uint32_t j;

    for (i = 0U; i < ms; i++)
    {
        for (j = 0U; j < 1700U; j++)
        {
            __NOP();
        }
    }
}

void DIAG_PinInit(void)
{
    /* 打开 GPIOC 时钟（重复打开无害） */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;                 /* 读回一次，等时钟稳定 */

    /* CRH 管 PC8..PC15，PC13 占 bit23..20：CNF=00（推挽输出）MODE=10（2 MHz） */
    GPIOC->CRH &= ~(0xFU << 20);
    GPIOC->CRH |= (0x2U << 20);
}

void DIAG_Blink(uint32_t times, uint32_t on_ms, uint32_t off_ms)
{
    uint32_t k;

    DIAG_PinInit();

    for (k = 0U; k < times; k++)
    {
        diag_toggle();
        diag_delay_ms(on_ms);
        diag_toggle();
        diag_delay_ms(off_ms);
    }
}

void DIAG_Panic(uint32_t code)
{
    uint32_t k;

    DIAG_PinInit();
    __disable_irq();

    for (;;)
    {
        for (k = 0U; k < code; k++)     /* 闪 code 下 */
        {
            diag_toggle();
            diag_delay_ms(110U);
            diag_toggle();
            diag_delay_ms(110U);
        }
        diag_delay_ms(700U);            /* 停顿，便于数清"闪了几下" */
    }
}

/* ---------------------------------------------------------------- 裸机自检 */
static void diag_send_str(const char *s)
{
    while (*s != '\0')
    {
        while ((USART1->SR & USART_SR_TXE) == 0U)
        {
        }
        USART1->DR = (uint8_t)(*s);
        s++;
    }
}

/* 打印一个无符号数（0~4294967295），不引 printf */
static void diag_send_u32(uint32_t v)
{
    char     buf[11];
    uint32_t n = 0U;

    if (v == 0U)
    {
        diag_send_str("0");
        return;
    }
    while ((v > 0U) && (n < sizeof(buf)))
    {
        buf[n] = (char)('0' + (v % 10U));
        v /= 10U;
        n++;
    }
    while (n > 0U)
    {
        n--;
        while ((USART1->SR & USART_SR_TXE) == 0U)
        {
        }
        USART1->DR = (uint8_t)buf[n];
    }
}

void DIAG_BareLoop(void)
{
    uint32_t k;
    uint32_t sec = 0U;

    DIAG_PinInit();

    /* PA9 = USART1_TX：CRH 管 PA8..PA15，PA9 占 bit7..4，CNF=10 AF 推挽 MODE=11 50MHz -> 0xB */
    RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN);
    (void)RCC->APB2ENR;
    GPIOA->CRH &= ~(0xFU << 4);
    GPIOA->CRH |= (0xBU << 4);

    USART1->BRR = 0x0457U;                              /* 8 MHz / 115200 = 69.4375 */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;

    diag_send_str("\r\n\r\n[BARE] no-OS diagnostic\r\n");
    diag_send_str("[BARE] if you can read this, MCU + clock + PA9 + USB-TTL are OK\r\n");
    diag_send_str("[BARE] HCLK = ");
    diag_send_u32(HAL_RCC_GetHCLKFreq());
    diag_send_str(" Hz\r\n");

    for (;;)
    {
        /* 每 1 s：LED 闪 1 下 + 串口发一行计数，证明在跑 */
        diag_toggle();
        diag_delay_ms(60U);
        diag_toggle();
        diag_delay_ms(440U);
        diag_toggle();
        diag_delay_ms(60U);
        diag_toggle();

        diag_send_str("[BARE] tick ");
        diag_send_u32(sec);
        diag_send_str("\r\n");
        sec++;

        diag_delay_ms(440U);
        (void)k;
    }
}
