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
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "pins.h"
#include "uart.h"

void init(void){
    /* outputs */
    DDRB = PIN_BIT(P_PWR) | PIN_BIT(P_DRV_MODE) | PIN_BIT(P_DRV_EN);
    DDRD = PIN_BIT(P_GLED);

    /* force on the voltage regulator */
    PIN_ON(P_PWR);

    /* use our pullup for the accelerometer's open-collector output */
    PIN_ON(P_ACC_INT);

    /* UART */
    uart_init();
}


int main(void){

    init();

    /* power off */
    PIN_OFF(P_PWR);

    /* show charging status (USB will hold the power on) */
    while(1){
        if(PIN_VALUE(P_CHARGE)){
            PIN_ON(P_GLED);
            puts_P(PSTR("FULL"));
        } else {
            PIN_TOGGLE(P_GLED);
            puts_P(PSTR("CHARGE"));
        }
        _delay_ms(500);
    }

    return 0;
}

