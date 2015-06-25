/*******************************************************************************
Copyright (C) 2015 OLogN Technologies AG

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*******************************************************************************/

#if !defined __HAL_PLATFORM_WIRING_MAIN_H__
#define __HAL_PLATFORM_WIRING_MAIN_H__

#if !defined ENERGIA
#include <avr/pgmspace.h>
#include "stdint.h"
#define ZEPTO_PROGMEM      __attribute__ ((progmem))
#define ZEPTO_MEMCPY_FROM_PROGMEM memcpy_PF
#else
#define ZEPTO_PROGMEM
#define ZEPTO_MEMCPY_FROM_PROGMEM( dest, src, len )\
{\
	uint16_t i; \
	for ( i=0; i<len; i++ )\
	((uint8_t*)dest)[i] = pgm_read_byte( ((uint8_t*)src) + i ); \
}
#endif

#include <Arduino.h>
#include "hal_time_conversions.h"

#define ZEPTO_PROGMEM_IN_USE

#define ZEPTO_PROG_CONSTANT_LOCATION ZEPTO_PROGMEM
#define ZEPTO_PROG_CONSTANT_READ_BYTE(x) pgm_read_byte(x)

#endif // __HAL_PLATFORM_WIRING_MAIN_H__
