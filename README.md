# STM32F030 as Capacitance Meter

Shows results in UART (115200 baud rate) and with LED blinking.

Results are 3 digits, similar to general capacitor marking: two first
digits are taken as is and the third means the number of zeroes (i.e. magnitude).

When button is pressed, result is in picofarads. When released, divide it by
100 to get picofarads (this mode is more suitable for small capacitors).

Led blinking code is as follows: dash means 3, dot means 1. So one just needs
sum dashes and dots. Exception is a single dash, meaning 0.

    1   *
    2   * *
    3   * * * (not the single, dash which is 0)
    4   *** *
    5   *** * *
    6   *** ***
    7   *** *** *
    8   *** *** * *
    9   *** *** ***
    0   *** (not 3, which is three dots)
