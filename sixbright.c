/* sixbright: replacement firmware for the HexBright flashlight
 *
 * Copyright (C) 2013 Tristan Willy <tristan.willy at gmail.com>
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
#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "pins.h"
#include "uart.h"

/* raw 8-bit ADC value when the temperature is too high
 * 85 = ~ 60 deg C
 */
#define OVERTEMP        85
/* reset value for ADC Status Register */
#define ADCSR_RESET     (_BV(ADEN) | _BV(ADIF) | _BV(ADPS2) | _BV(ADPS1))

/* system state machine */
enum state {
    STATE_LOW,
    STATE_MED,
    STATE_HIGH,
    STATE_OFF
};

/* debounced button */
static volatile uint8_t rled_sw, rled_cnt_down, rled_cnt_up;
/* true if USB is connected */
static bool on_usb;


/* interrupt when the button changes */
ISR(INT0_vect){
    /* start timer 2 to capture the button in the future */
    TIMSK2 = _BV(TOIE2);
}


/* timer 2 overflow */
ISR(TIMER2_OVF_vect){
    uint8_t new_sw;

    /* ~ 8ms has passed */
    new_sw = PIN_VALUE(P_RLED_SW);
    if(new_sw != rled_sw){
        rled_sw = new_sw;
        if(rled_sw){
            rled_cnt_down++;
        } else {
            rled_cnt_up++;
        }
    }
    TIMSK2 = 0;
}


/* sample the currently selected analog channel
 * returns an 8 bit conversion result
 */
uint8_t adc_sample(void){
    ADCSRA = ADCSR_RESET | _BV(ADSC);
    loop_until_bit_is_set(ADCSRA, ADIF);

    return ADCH;
}


void init(void){
    uint8_t adc;

    /* outputs */
    DDRB = PIN_BIT(P_PWR) | PIN_BIT(P_DRV_MODE) | PIN_BIT(P_DRV_EN);
    DDRD = PIN_BIT(P_TX) | PIN_BIT(P_GLED);

    /* force on the voltage regulator */
    PIN_ON(P_PWR);

    /* use our pullup for the accelerometer's open-collector output */
    PIN_ON(P_ACC_INT);

    /* watch the button switch with an interrupt */
    EICRA = _BV(ISC00);
    EIMSK = _BV(INT0);
    TCCR2B = _BV(CS22) | _BV(CS21);
    rled_sw = PIN_VALUE(P_RLED_SW);

    /* on_usb detection: use MCP73831's shutdown status */
    ADMUX = _BV(REFS0) | _BV(ADLAR) | _BV(MUX1) | _BV(MUX0);
    adc = adc_sample();
    if(adc >= 153 && adc <= 187){
        /* Hi-Z within 10% */
        on_usb = false;
    } else {
        on_usb = true;
    }

    /* temperature sensor */
    DIDR0 = PIN_BIT(P_TEMP);
    ADMUX = _BV(REFS0) | _BV(ADLAR);
    ADCSRA = ADCSR_RESET;

    /* UART */
    uart_init();

    /* enable interrupts */
    sei();
}


int main(void){
    enum state state;
    uint8_t last_down;

    init();

    /* inital state depends on who started us */
    if(on_usb){
        state = STATE_OFF;
    } else {
        /* verify a solid button press to power on */
        _delay_ms(8);
        if(!PIN_VALUE(P_RLED_SW)){
            /* nope. power off */
            state = STATE_OFF;
        } else {
            /* ok, let's turn on low */
            state = STATE_LOW;
        }
    }

    /* main loop */
    do {
        last_down = rled_cnt_down;

        /* act */
        switch(state){
        case STATE_LOW:
            PIN_ON(P_PWR);
            PIN_OFF(P_DRV_MODE);
            PIN_ON(P_DRV_EN);
            /* shortcut to OFF until we get PWM working */
            state = STATE_OFF;
            break;
        case STATE_MED:
            PIN_OFF(P_DRV_MODE);
            PIN_ON(P_DRV_EN);
            state = STATE_HIGH;
            break;
        case STATE_HIGH:
            PIN_OFF(P_DRV_MODE);
            PIN_ON(P_DRV_EN);
            state = STATE_OFF;
            break;
        case STATE_OFF:
            /* power off */
            PIN_OFF(P_PWR);
            /* if we keep running, then we're on USB */
            _delay_ms(250);
            PIN_OFF(P_DRV_EN);
            on_usb = true;
            state = STATE_LOW;
            break;
        }

        /* wait for a button event */
        while(last_down == rled_cnt_down){
            /* a charging battery means USB was attached */
            if(!PIN_VALUE(P_CHARGE)){
                on_usb = true;
            }

            /* monitoring code */
            if(on_usb){
                printf_P(PSTR("T %d\n"), adc_sample());
                if(PIN_VALUE(P_CHARGE)){
                    PIN_ON(P_GLED);
                    puts_P(PSTR("FULL"));
                } else {
                    PIN_TOGGLE(P_GLED);
                    puts_P(PSTR("CHARGE"));
                }
            }
        }
    } while(1);

    return 0;
}

