#include "stm32f030.h"

#define SYS_CLK_MHZ 8

char hex[] = "0123456789ABCDEF";

int intDiv(int a, int b) {
    int res = 0;
    int power = 1;
    while (a - b >= b) {
        b <<= 1;
        power <<= 1;
    }
    while (power > 0) {
        if (a - b >= 0) {
            a -= b;
            res += power;
        }
        b >>= 1;
        power >>= 1;
    }
    return res;
}

void uartEnable(int divisor) {
    REG_L(GPIOA_BASE, GPIO_MODER) &= ~(3 << (9 * 2));
    REG_L(GPIOA_BASE, GPIO_MODER) |= (2 << (9 * 2));
    REG_L(GPIOA_BASE, GPIO_AFRH) &= ~(0xF << ((9 - 8) * 4));
    REG_L(GPIOA_BASE, GPIO_AFRH) |= (1 << ((9 - 8) * 4));
    REG_L(GPIOA_BASE, GPIO_MODER) &= ~(3 << (10 * 2));
    REG_L(GPIOA_BASE, GPIO_MODER) |= (2 << (10 * 2));
    REG_L(GPIOA_BASE, GPIO_AFRH) &= ~(0xF << ((10 - 8) * 4));
    REG_L(GPIOA_BASE, GPIO_AFRH) |= (1 << ((10 - 8) * 4));
    REG_L(RCC_BASE, RCC_APB2ENR) |= (1 << 14);
    REG_L(USART_BASE, USART_BRR) = divisor;
    REG_L(USART_BASE, USART_CR1) |= 1;
    REG_L(USART_BASE, USART_CR1) |= (1 << 3);
}

void send(int c) {
    REG_L(USART_BASE, USART_TDR) = c;
    while ((REG_L(USART_BASE, USART_ISR) & (1 << 6)) == 0);
}

void sends(char* s) {
    while (*s) {
        send(*(s++));
    }
}

void sendHex(int x, int d) {
    while (d-- > 0) {
        send(hex[(x >> (d * 4)) & 0xF]);
    }
}

void sendDec(int x) {
    static char s[10];
    int i, x1;
    i = 0;
    while (x > 0) {
        x1 = intDiv(x, 10);
        s[i++] = x - x1 * 10;
        x = x1;
    }
    if (i == 0) {
        s[i++] = 0;
    }
    while (i > 0) {
        send('0' + s[--i]);
    }
}

void setupPorts() {
    char i;
    REG_L(RCC_BASE, RCC_AHBENR) |= (1 << 17) | (1 << 18); //port A, B
}

void setupTimer3() {
    REG_L(GPIOA_BASE, GPIO_MODER) &= ~(3 << (6 * 2));
    REG_L(GPIOA_BASE, GPIO_MODER) |= (2 << (6 * 2)); // alternate function for pa6
    REG_L(GPIOA_BASE, GPIO_AFRL) &= ~(15 << (6 * 4));
    REG_L(GPIOA_BASE, GPIO_AFRL) |= (1 << (6 * 4)); // alternate function 1 (tim3_ch1)
    
    REG_L(RCC_BASE, RCC_APB1ENR) |= (1 << 1); // enable clock to timer 3
    REG_L(TIM3_BASE, TIM_CCR1) = 1; // trigger from TI1, no filter
    REG_L(TIM3_BASE, TIM_CCER) = 0; // rising edges
    REG_L(TIM3_BASE, TIM_SMCR) = 0; // external clock off
    REG_L(TIM3_BASE, TIM_SMCR) = 0x150; // external source - filtered trigger TI1
    REG_L(TIM3_BASE, TIM_SMCR) |= 7; // external clock
    REG_L(TIM3_BASE, TIM_CR2) = 0; // TI1 from tim3_ch1
    REG_L(TIM3_BASE, TIM_ARR) = 0xFFFF; // max value
    REG_L(TIM3_BASE, TIM_PSC) = 0;
    REG_L(TIM3_BASE, TIM_CNT) = 0;
    REG_L(TIM3_BASE, TIM_CR1) = 1; // enable timer
}

void setupTimer1() {
    REG_L(RCC_BASE, RCC_APB2ENR) |= (1 << 11); // enable clock to timer 1
    REG_L(TIM1_BASE, TIM_SMCR) = 0; // external clock off
    REG_L(TIM1_BASE, TIM_CR2) = 0;
    REG_L(TIM1_BASE, TIM_ARR) = 0xFFFF; // max value
    REG_L(TIM1_BASE, TIM_PSC) = 7999;
    REG_L(TIM1_BASE, TIM_EGR) = 1; // manual update event
    REG_L(TIM1_BASE, TIM_SR) = 0;
    REG_L(TIM1_BASE, TIM_CNT) = 0;
    REG_L(TIM1_BASE, TIM_CR1) = 5; // enable timer
}

void countForPeriod(int period) {
    int psc = 1;
    while (period > 10000) {
        period = intDiv(period, 10);
        psc *= 10;
    }
    REG_L(TIM1_BASE, TIM_ARR) = period - 1;
    REG_L(TIM1_BASE, TIM_PSC) = psc - 1;
    REG_L(TIM1_BASE, TIM_CNT) = 0;
    REG_L(TIM1_BASE, TIM_CR1) = 1; // enable timer
}

int main(void) {
    int i, a;
    
    setupPorts();
    
    uartEnable(SYS_CLK_MHZ * 1000000 / 115200);
    
    setupTimer3();
    setupTimer1();

    while(1) {
        sendDec(REG_L(TIM1_BASE, TIM_CNT));
        send(' ');
        sendHex(REG_L(TIM1_BASE, TIM_SR), 4);
        sends("\r\n");
        for (i = 0; i < 100000; i++) {
        }
    }
}
