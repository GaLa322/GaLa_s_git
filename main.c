#include <stdint.h>

/* ======================= 模块 1：寄存器地址 ======================= */
#define APB2_BASE        0x40010000UL     /* APB2 外设基地址 */
#define AHB_BASE         0x40020000UL     /* AHB  外设基地址 */
#define GPIOA_BASE       (APB2_BASE + 0x0800UL)   /* 0x40010800 */
#define GPIOB_BASE       (APB2_BASE + 0x0C00UL)   /* 0x40010C00 */
#define GPIOC_BASE       (APB2_BASE + 0x1000UL)   /* 0x40011000 */
#define RCC_BASE         (AHB_BASE  + 0x1000UL)   /* 0x40021000 */
#define SYSTICK_BASE     0xE000E010UL             /* 内核 SysTick */

/* ======================= 模块 2：寄存器结构体 ======================= */
typedef struct {
    volatile uint32_t CRL;    /* 0x00 端口配置低寄存器 (Px0~Px7)  */
    volatile uint32_t CRH;    /* 0x04 端口配置高寄存器 (Px8~Px15) */
    volatile uint32_t IDR;    /* 0x08 端口输入数据寄存器          */
    volatile uint32_t ODR;    /* 0x0C 端口输出数据寄存器          */
    volatile uint32_t BSRR;   /* 0x10 端口位设置/清除寄存器       */
    volatile uint32_t BRR;    /* 0x14 端口位清除寄存器            */
    volatile uint32_t LCKR;   /* 0x18 端口配置锁定寄存器          */
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CTRL;   /* 0x00 控制及状态寄存器 */
    volatile uint32_t LOAD;   /* 0x04 重装载数值寄存器 */
    volatile uint32_t VAL;    /* 0x08 当前数值寄存器   */
    volatile uint32_t CALIB;  /* 0x0C 校准数值寄存器   */
} SysTick_Type;

#define GPIOA            ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB            ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC            ((GPIO_TypeDef *) GPIOC_BASE)
#define SysTick          ((SysTick_Type *) SYSTICK_BASE)

#define RCC_CR           (*(volatile uint32_t *)(RCC_BASE + 0x00UL))
#define RCC_CFGR         (*(volatile uint32_t *)(RCC_BASE + 0x04UL))
#define RCC_APB2ENR      (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

/* ======================= 模块 3：LED 定义 ======================= */
#define LED_NUM          4U                              /* 共 4 个灯 */

#define LED_R_PORT       GPIOA                           /* 外接红灯 */
#define LED_R_PIN        0U                              /* PA0 */
#define LED_G_PORT       GPIOB                           /* 外接绿灯 */
#define LED_G_PIN        0U                              /* PB0 */
#define LED_B_PORT       GPIOB                           /* 外接蓝灯 */
#define LED_B_PIN        1U                              /* PB1 */
#define LED_BOARD_PORT   GPIOC                           /* 板载灯   */
#define LED_BOARD_PIN    13U                             /* PC13     */

/* ======================= 模块 4：驱动函数 ======================= */

/* 配置引脚为输出：pin<8 用 CRL，pin>=8 用 CRH，每引脚 4 位 = CNF(2) + MODE(2) */
void GPIO_ConfigPin(GPIO_TypeDef *port, uint8_t pin, uint32_t cnf, uint32_t mode)
{
    volatile uint32_t *reg;
    uint32_t shift, tmp;

    if (pin < 8U) { reg = &port->CRL; shift = (uint32_t)pin * 4U; }
    else          { reg = &port->CRH; shift = ((uint32_t)pin - 8U) * 4U; }

    tmp  = *reg;
    tmp &= ~(0xFU << shift);                  /* 清掉该引脚 4 位配置 */
    tmp |= ((cnf << 2) | mode) << shift;      /* 写入 CNF 与 MODE    */
    *reg = tmp;
}

/* 引脚输出高电平：写 BSRR 低 16 位 */
static void Pin_Set(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRR = (1UL << pin);
}

/* 引脚输出低电平：写 BRR（等价于写 BSRR 高 16 位） */
static void Pin_Reset(GPIO_TypeDef *port, uint8_t pin)
{
    port->BRR = (1UL << pin);
}

/* 把第 idx 个灯点亮（0=红 1=绿 2=蓝 3=板载） */
static void LED_On(uint32_t idx)
{
    switch (idx)
    {
        case 0U: Pin_Set  (LED_R_PORT,     LED_R_PIN);     break;  /* PA0 高电平点亮 */
        case 1U: Pin_Set  (LED_G_PORT,     LED_G_PIN);     break;  /* PB0 高电平点亮 */
        case 2U: Pin_Set  (LED_B_PORT,     LED_B_PIN);     break;  /* PB1 高电平点亮 */
        case 3U: Pin_Reset(LED_BOARD_PORT, LED_BOARD_PIN); break;  /* PC13 低电平点亮 */
        default: break;
    }
}

/* 把第 idx 个灯熄灭（与 LED_On 逻辑相反） */
static void LED_Off(uint32_t idx)
{
    switch (idx)
    {
        case 0U: Pin_Reset(LED_R_PORT,     LED_R_PIN);     break;
        case 1U: Pin_Reset(LED_G_PORT,     LED_G_PIN);     break;
        case 2U: Pin_Reset(LED_B_PORT,     LED_B_PIN);     break;
        case 3U: Pin_Set  (LED_BOARD_PORT, LED_BOARD_PIN); break;  /* 高电平熄灭 */
        default: break;
    }
}

uint32_t HCLK_GetHz(void)
{
    const uint16_t ahb_div[16] = { 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U,
                                   2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U };
    uint32_t cfgr = RCC_CFGR;
    uint32_t sysclk;

    switch ((cfgr >> 2) & 0x3UL)                  /* SWS：当前时钟源 */
    {
        case 0x2UL:                               /* PLL 输出 */
        {
            uint32_t mul_code = (cfgr >> 18) & 0xFUL;
            uint32_t mul      = (mul_code == 0xFUL) ? 16UL : (mul_code + 2UL);
            uint32_t src      = ((cfgr >> 16) & 0x1UL)
                              ? (8000000UL >> ((cfgr >> 17) & 0x1UL))   /* HSE */
                              : 4000000UL;                              /* HSI/2 */
            sysclk = src * mul;
            break;
        }
        case 0x1UL:  sysclk = 8000000UL; break;   /* HSE 直接作为系统时钟 */
        default:     sysclk = 8000000UL; break;   /* HSI 8MHz（复位默认） */
    }

    return sysclk / (uint32_t)ahb_div[(cfgr >> 4) & 0xFUL];
}

void Delay_ms(uint32_t ms)
{
    uint32_t i;
    uint32_t ticks = HCLK_GetHz() / 1000UL;   /* 1ms 需要多少计数 */

    for (i = 0U; i < ms; i++)
    {
        SysTick->LOAD = ticks - 1UL;
        SysTick->VAL  = 0UL;
        SysTick->CTRL = 0x00000005UL;         /* CLKSOURCE=1, ENABLE=1 */
        while ((SysTick->CTRL & 0x00010000UL) == 0UL) { }  
    }
    SysTick->CTRL = 0UL;
    SysTick->VAL  = 0UL;
}

/* ======================= 模块 5：主函数 ======================= */
int main(void)
{
    uint32_t i;

    /* 5.1 使能 GPIOA / GPIOB / GPIOC 时钟（不使能时钟写寄存器无效） */
    RCC_APB2ENR |= (1UL << 2) | (1UL << 3) | (1UL << 4);

    /* 5.2 配置 4 个 LED 引脚为通用推挽输出*/
    GPIO_ConfigPin(LED_R_PORT,     LED_R_PIN,     0U, 3U);
    GPIO_ConfigPin(LED_G_PORT,     LED_G_PIN,     0U, 3U);
    GPIO_ConfigPin(LED_B_PORT,     LED_B_PIN,     0U, 3U);
    GPIO_ConfigPin(LED_BOARD_PORT, LED_BOARD_PIN, 0U, 2U);

    /* 5.3 上电全灭，避免上电瞬间出现随机亮灯状态 */
    for (i = 0U; i < LED_NUM; i++)
    {
        LED_Off(i);
    }

    /* 5.4 主循环：4 个灯依次点亮 1 秒，形成流水效果 */
    while (1)
    {
        for (i = 0U; i < LED_NUM; i++)
        {
            LED_On(i);          /* 点亮当前灯   */
            Delay_ms(1000);     /* 保持 1 秒    */
            LED_Off(i);         /* 熄灭当前灯，进入下一个 */
        }
    }
}
