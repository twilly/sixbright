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
#include <avr/wdt.h>
#include <util/delay.h>
#include "pins.h"
#include "uart.h"

/* raw 8-bit ADC value when the temperature is too high
 * 85 = ~ 60 deg C
 */
#define OVERTEMP        85
/* reset value for ADC Status Register */
#define ADCSR_RESET     (_BV(ADEN) | _BV(ADIF) | _BV(ADPS2) | _BV(ADPS1))
/* ADC MUX selection */
#define ADC_TEMP        0
#define ADC_LSTATUS     3
/* reset value for the Power Reduction Register */
#define PRR_RESET       _BV(PRSPI)
/* top value for PWM (~ 150hz @ 8mhz P&F correct) */
#define PWM_TOP         26666
/* approximate ticks per second */
#define TICKS_PER_SEC   31
/* approximate ticks per half-second */
#define TICKS_PER_HSEC  15

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
/* system tick @ ~ 30.5hz (8mhz / 1024 / 256) */
static volatile uint8_t tick;


/* interrupt when the button changes */
ISR(INT0_vect){
    /* start timer 2 to capture the button in the future */
    TIMSK2 = _BV(TOIE2);
}


/* timer 0 overflow - system tick */
ISR(TIMER0_OVF_vect){
    tick++;
}


/* timer 2 overflow - button interrupt triggered */
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
 * this function is optimized for very low frequency sampling (power saving)
 * returns an 8 bit conversion result
 */
uint8_t adc_sample(uint8_t mux){
    uint8_t sample;

    /* power up ADC */
    PRR = PRR_RESET;

    /* sample */
    ADMUX = _BV(REFS0) | _BV(ADLAR) | mux;
    ADCSRA = ADCSR_RESET | _BV(ADSC);
    loop_until_bit_is_set(ADCSRA, ADIF);
    sample = ADCH;

    /* shutdown ADC */
    ADCSRA = 0;
    PRR |= _BV(PRADC);

    return sample;
}


/* start PWM on OC1B */
void pwm_on(uint16_t level){
    /* clamp level and set */
    level = level > PWM_TOP ? PWM_TOP : level;
    OCR1B = level;
    /* clear counter */
    TCNT1 = 0;
    /* clear OC1B */
    PORTB &= ~ _BV(2);
    /* start clocking */
    TCCR1A = _BV(COM1B1) | _BV(WGM10);
    TCCR1B = _BV(WGM13) | _BV(CS10);
}


/* stop PWM on OC1B
 * note: OC1B's level not set
 */
void pwm_off(void){
    TCCR1A = 0;
    TCCR1B = 0;
}


/* difference between two ticks */
uint8_t tick_diff(uint8_t start, uint8_t now){
    if(now < start){
        return (UINT8_MAX - start) + now + 1;
    }
    return now - start;
}


/* delay for count ticks */
void tick_delay(uint8_t ticks){
    uint8_t start = tick;
    while(tick_diff(start, tick) < ticks){
        wdt_reset();
    }
}


void init(void){
    uint8_t adc;

    /* outputs */
    DDRB = PIN_BIT(P_PWR) | PIN_BIT(P_DRV_MODE) | PIN_BIT(P_DRV_EN);
    DDRD = PIN_BIT(P_TX) | PIN_BIT(P_GLED);

    /* force on the voltage regulator */
    PIN_ON(P_PWR);

    /* use pullups on AIN1, AIN2, and DIN1 pins because they're floating */
    PIN_ON(P_DIN1);
    PIN_ON(P_AIN1);
    PIN_ON(P_AIN2);

    /* use our pullup for the accelerometer's open-collector output */
    PIN_ON(P_ACC_INT);

    /* watch the button switch with an interrupt */
    EICRA = _BV(ISC00);
    EIMSK = _BV(INT0);
    TCCR2B = _BV(CS22) | _BV(CS21);
    rled_sw = PIN_VALUE(P_RLED_SW);

    /* on_usb detection: use MCP73831's shutdown status */
    adc = adc_sample(ADC_LSTATUS);
    if(adc >= 153 && adc <= 187){
        /* Hi-Z within 10% */
        on_usb = false;
    } else {
        on_usb = true;
    }

    /* UART */
    uart_init();

    /* PWM TOP value */
    OCR1A = PWM_TOP;

    /* system clock */
    TCCR0B = _BV(CS02) | _BV(CS00);
    TIMSK0 = _BV(TOIE0);

    /* shut off units we're not using */
    DIDR0 = PIN_BIT(P_TEMP);
    PRR = PRR_RESET;
    ACSR = _BV(ACD);

    /* enable interrupts */
    sei();
}


/* set a brightness on the light */
void light_set(enum state state){
    switch(state){
    case STATE_LOW:
        PIN_OFF(P_DRV_MODE);
        pwm_on(PWM_TOP / 10);
        break;
    case STATE_MED:
        PIN_OFF(P_DRV_MODE);
        pwm_off();
        PIN_ON(P_DRV_EN);
        break;
    case STATE_HIGH:
        PIN_ON(P_DRV_MODE);
        pwm_off();
        PIN_ON(P_DRV_EN);
        break;
    case STATE_OFF:
        pwm_off();
        PIN_OFF(P_DRV_EN);
        break;
    }
}


/* idle: call when the main loop is blocked
 * c_state := current state
 * n_state := next state
 * returns next state which may or may not be the same as n_state
 */
enum state idle(enum state c_state, enum state n_state){
    /* static and first-call initalized variables */
    static bool initalized = false;
    static uint8_t last_report, last_temp_sample, temperature;
    /* per-call variables */
    uint8_t i;

    /* reset watchdog */
    wdt_reset();

    /* first run? init! */
    if(!initalized){
        last_temp_sample = last_report = tick;
        temperature = adc_sample(ADC_TEMP);
        initalized = true;
    }

    /* a charging battery means USB was attached */
    if(!PIN_VALUE(P_CHARGE)){
        on_usb = true;
    }

    /* sample temperature every 5 seconds */
    if(tick_diff(last_temp_sample, tick) >= (5 * TICKS_PER_SEC)){
        last_temp_sample = tick;
        temperature = adc_sample(ADC_TEMP);
    }

    /* over temperature */
    if((c_state == STATE_HIGH || c_state == STATE_MED) &&
            temperature > OVERTEMP){
        /* We're too hot! Blink at medium, go into low light, and
         * have have the next button press power off.
         */
        light_set(STATE_MED);
        for(i = 0; i < 6; i++){
            tick_delay(TICKS_PER_HSEC);
            PIN_TOGGLE(P_DRV_EN);
        }
        light_set(STATE_LOW);
        c_state = STATE_LOW;
        n_state = STATE_OFF;
    }

    /* monitoring code */
    if(on_usb && tick_diff(last_report, tick) >= TICKS_PER_HSEC){
        printf_P(PSTR("T %d\n"), temperature);
        if(PIN_VALUE(P_CHARGE)){
            PIN_ON(P_GLED);
            puts_P(PSTR("FULL"));
        } else {
            PIN_TOGGLE(P_GLED);
            puts_P(PSTR("CHARGE"));
        }
        last_report = tick;
    }

    return n_state;
}


/* enter_state: switch into a new state
 * state := state to enter
 * returns the next state to enter
 */
enum state enter_state(enum state state){
    enum state n_state;

    light_set(state);
    switch(state){
    case STATE_LOW:
        n_state = STATE_MED;
        PIN_ON(P_PWR);
        break;
    case STATE_MED:
        n_state = STATE_HIGH;
        break;
    case STATE_HIGH:
        n_state = STATE_OFF;
        break;
    case STATE_OFF:
        n_state = STATE_LOW;
        /* power off */
        PIN_OFF(P_PWR);
        /* wait until the button is lifted */
        while(PIN_VALUE(P_RLED_SW)){
            wdt_reset();
        }
        tick_delay(TICKS_PER_HSEC);
        /* if we're still running, then we're USB powered */
        on_usb = true;
        break;
    }

    return n_state;
}


int main(void){
    enum state c_state, n_state;
    uint8_t last_down;

    /* enable the watchdog */
    wdt_enable(WDTO_1S);

    /* init */
    init();

    /* inital state depends on who started us */
    if(on_usb){
        c_state = STATE_OFF;
    } else {
        /* verify a solid button press to power on */
        _delay_ms(2);
        if(!PIN_VALUE(P_RLED_SW)){
            /* nope. power off */
            c_state = STATE_OFF;
        } else {
            /* ok, let's turn on low */
            c_state = STATE_LOW;
        }
    }

    /* enter our first state */
    n_state = enter_state(c_state);

    /* reset watchdog */
    wdt_reset();

    /* main loop */
    do {
        /* wait for a button event */
        last_down = rled_cnt_down;
        while(last_down == rled_cnt_down){
            /* do idle tasks */
            n_state = idle(c_state, n_state);
        }

        /* switch states (current state = next state) */
        c_state = n_state;
        n_state = enter_state(c_state);
    } while(1);

    return 0;
}

