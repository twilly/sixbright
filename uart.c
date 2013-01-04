/* uart.c: AVR UART driver for avr-libc
 *
 * Copyright (C) 2012-2013 Tristan Willy <tristan.willy at gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <avr/io.h>
#include <util/setbaud.h>

static int uart_putchar(char c, FILE *stream);
static int uart_getchar(FILE *stream);

static FILE uart_stdio = FDEV_SETUP_STREAM(uart_putchar, uart_getchar,
        _FDEV_SETUP_RW);

void uart_init(void){
    /* serial port */
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X == 1
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(U2X0);
#else
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
#endif
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

    /* link serial port to stdin/out */
    stdin  = &uart_stdio;
    stdout = &uart_stdio;
}


static int uart_putchar(char c, FILE *stream){
    if(c == '\n'){
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    return 0;
}


static int uart_getchar(FILE *stream){
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

