/* config.h: compile-time configuration for sixbright firmware
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
#ifndef CONFIG_H
#define CONFIG_H

/* raw 8-bit ADC value when the temperature is too high
 * 85 = ~ 60 deg C
 */
#define CONFIG_OVERTEMP                     85

/* number of seconds to count as a long button press
 * maximum number of seconds is 256 / ticks per second (~ 8 seconds)
 */
#define CONFIG_LONG_BUTTON_DOWN_TIME        1.0f

/* strobe frequency (hertz) */
#define CONFIG_STROBE_HZ                    12

#endif /* CONFIG_H */

