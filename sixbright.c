/* sixbright: replacement firmware for the HexBright flashlight
 *
 * Copyright (C) 2012 Tristan Willy <tristan.willy at gmail.com>
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
#include <avr/io.h>
#include <util/delay.h>

/*** Pins ***/
/* Red LED and Switch (in/out) */
#define P_RLED_SW_PORT          D
#define P_RLED_SW_PIN           2
/* Accelerometer Interrupt (in) */
#define P_ACC_INT_PORT          D
#define P_ACC_INT_PIN           3
/* Green LED (out) */
#define P_GLED_PORT             D
#define P_GLED_PIN              5
/* NOT ON SCHEMATIC: PGOOD (in) */
#define P_PGOOD_PORT            D
#define P_PGOOD_PIN             7
/* Power (out) */
#define P_PWR_PORT              B
#define P_PWR_PIN               0
/* Driver Mode (out, feedback voltage) */
#define P_DRV_MODE_PORT         B
#define P_DRV_MODE_PIN          1
/* Driver Enable (out) */
#define P_DRV_EN_PORT           B
#define P_DRV_EN_PIN            2
/* Temperature (in, analog) */
#define P_TEMP_PORT             C
#define P_TEMP_PIN              0
/* Charge (in, mixed digital/analog) */
#define P_CHARGE_PORT           C
#define P_CHARGE_PIN            3

/** pin macros **/
/* pin support: bit and PORT/PIN expansion */
#define _PIN_PASTE2(a, b)       a ## b
#define _PIN_PASTE(a, b)        _PIN_PASTE2(a, b)
#define PIN_BIT(p)              _BV(p##_PIN)
#define PIN_PORT(p)             _PIN_PASTE(PORT, _PIN_PASTE(p, _PORT))
#define PIN_PIN(p)              _PIN_PASTE(PIN, _PIN_PASTE(p, _PORT))
/* pin interface */
#define PIN_ON(p)               PIN_PORT(p) |= PIN_BIT(p)
#define PIN_OFF(p)              PIN_PORT(p) &= ~ PIN_BIT(p)
#define PIN_TOGGLE(p)           PIN_PIN(p)   = PIN_BIT(p)
#define PIN_VALUE(p)            (PIN_PIN(p) & PIN_BIT(p))


void init(void){
    /* outputs */
    DDRB = PIN_BIT(P_PWR) | PIN_BIT(P_DRV_MODE) | PIN_BIT(P_DRV_EN);
    DDRD = PIN_BIT(P_GLED);

    /* force on the voltage regulator */
    PIN_ON(P_PWR);

    /* use our pullup for the accelerometer's open-collector output */
    PIN_ON(P_ACC_INT);
}


int main(void){
    int i;

    init();

    /* blink the green LED for 10 seconds */
    for(i = 0; i < 10; i++){
        PIN_TOGGLE(P_GLED);
        _delay_ms(500);
    }

    /* power off */
    PIN_OFF(P_PWR);

    /* toggle forever (external power may be holding us up) */
    while(1){
        PIN_TOGGLE(P_GLED);
        _delay_ms(500);
    }

    return 0;
}

