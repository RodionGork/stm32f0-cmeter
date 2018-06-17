#include "stm32f030.h"

#define SYS_CLK_MHZ 12
#define UART_BAUD_RATE 115200

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

void setupClock() {
    REG_L(RCC_BASE, RCC_CR) |= (1 << 16); // HSEON = 1
    while ((REG_L(RCC_BASE, RCC_CR) & (1 << 17)) == 0);
    REG_L(RCC_BASE, RCC_CFGR) |= (1 << 0);
    while (((REG_L(RCC_BASE, RCC_CFGR) >> 2) & 0x3) != 1);
}

void setupPorts() {
    char i;
    REG_L(RCC_BASE, RCC_AHBENR) |= (1 << 17); // enable port A
    REG_L(GPIOA_BASE, GPIO_MODER) |= (1 << (5 * 2)); // pa5 - output (LED)
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
    REG_L(TIM1_BASE, TIM_CNT) = 0;
    REG_L(TIM1_BASE, TIM_CR1) = 5; // enable timer
}

int countForPeriod(int period) {
    int res;
    int psc = 1;
    while (period > 10000) {
        period = intDiv(period, 10);
        psc *= 10;
    }
    REG_L(TIM1_BASE, TIM_ARR) = period - 1;
    REG_L(TIM1_BASE, TIM_PSC) = psc - 1;
    REG_L(TIM1_BASE, TIM_EGR) = 1; // manual update event

    // stop and reload timers
    REG_L(TIM1_BASE, TIM_CR1) &= ~1;
    REG_L(TIM3_BASE, TIM_CR1) &= ~1;
    REG_L(TIM1_BASE, TIM_SR) = 0;
    REG_L(TIM1_BASE, TIM_CNT) = 0;
    REG_L(TIM3_BASE, TIM_CNT) = 0;

    REG_L(TIM3_BASE, TIM_CR1) |= 1;
    REG_L(TIM1_BASE, TIM_CR1) |= 1;

    while ((REG_L(TIM1_BASE, TIM_SR) & 1) == 0) {
    }
    return REG_L(TIM3_BASE, TIM_CNT);
}

void ledControl(char state) {
    REG_L(GPIOA_BASE, GPIO_BSRR) |= (1 << (5 + (state ? 0 : 16)));
}

int printResult(int ticks, int count) {
    int res;
    sendDec(ticks);
    sends(": ");
    sendDec(count);
    sends(" = ");
    if (count > 0) {
        if (ticks > 1000000) {
            res = intDiv(intDiv(ticks, 14) * 125, count);
        } else {
            res = intDiv(ticks * 125, count * 14);
        }
    } else {
        res = 0;
    }
}

void delay(int d) {
    d *= 30000;
    while (d > 0) {
        d -= 1;
    }
}

void blink(int d) {
    ledControl(1);
    delay(d);
    ledControl(0);
}

void blinkDigit(int d) {
    send('0' + d);
    if (d == 0) {
        blink(30);
        delay(30);
        return;
    }
    if (d > 3) {
        while (d >= 3) {
            blink(30);
            delay(10);
            d -= 3;
        }
    }
    while (d > 0) {
        blink(10);
        delay(10);
        d -= 1;
    }
    delay(20);
}

void blinkResult(int x) {
    int c, m = 0;
    while (x > 100) {
        x = intDiv(x, 10);
        m += 1;
    }
    c = intDiv(x, 10);
    blinkDigit(c);
    blinkDigit(x - c * 10);
    blinkDigit(m);
    sends("\r\n");
}

int main(void) {
    int i, a;
    
    setupClock();
    
    setupPorts();
    
    uartEnable(SYS_CLK_MHZ * 1000000 / UART_BAUD_RATE);
    
    setupTimer3();
    setupTimer1();
    
    sends("C-meter. Results are in pF,\r\nthe last digit is magnitude.\r\nDivide by 100 if button not pressed.");

    while(1) {
        a = 100;
        do {
            a *= 10;
            i = countForPeriod(a);
        } while (a <= 1000000 && i < 1000);
        a = printResult(a, i);
        blinkResult(a);
        delay(70);
    }
}

