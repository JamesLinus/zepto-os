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

#include "../../hal_commlayer.h"
#include "../../hal_waiting.h"

#define MAX_PACKET_SIZE 50
#define START_OF_PACKET 0x01
#define END_OF_TRANSMISSION 0x17
#define BYTE_MARKER 0xFF

uint8_t hal_wait_for( waiting_for* wf )
{
    for (;;)
    {
        if (wf->wait_packet && Serial.available())
        {
            if (Serial.read() == START_OF_PACKET)
            {
                return WAIT_RESULTED_IN_PACKET;
            }
        }
    }

    return WAIT_RESULTED_IN_FAILURE;
}

uint8_t wait_for_timeout( uint32_t timeout)
{
    ZEPTO_DEBUG_ASSERT(0);
    return 0;
}

uint8_t hal_get_packet_bytes( MEMORY_HANDLE mem_h )
{
    uint8_t buffer[ MAX_PACKET_SIZE ];
    uint8_t i, byte, marker_detected = 0;
    uint32_t timeout = 2000; // in ms
    uint32_t start_reading  = getTime();
    while ((start_reading + timeout) > getTime() && i < MAX_PACKET_SIZE)
    {
        if (!Serial.available())
            continue;

        byte = Serial.read();

        if (byte == BYTE_MARKER)
        {
            marker_detected = true;
            continue;
        }
        else if (byte == END_OF_TRANSMISSION)
        {
            ZEPTO_DEBUG_ASSERT( i && i <= MAX_PACKET_SIZE );
            zepto_write_block( mem_h, buffer, i );
            return WAIT_RESULTED_IN_PACKET;
        }
        else if (marker_detected)
        {
            uint8_t value = 0;
            switch (byte) {
                case 0x00:
                    value = 0x01;
                    break;
                case 0x02:
                    value = 0x17;
                    break;
                case 0x03:
                    value = 0xFF;
                    break;
                default:
                    return WAIT_RESULTED_IN_FAILURE;
            }
            buffer[i++] = value;
            marker_detected = false;
        }
        else
        {
            buffer[i++] = byte;
        }
    }

    return WAIT_RESULTED_IN_FAILURE;
}

bool communication_initialize()
{
    Serial.begin(9600);
    while (!Serial) {
        // wait for serial port to connect. Needed for Leonardo only
    }
    return true;
}

uint8_t send_message( MEMORY_HANDLE mem_h )
{
    uint8_t i = 0;
    uint16_t sz = memory_object_get_request_size( mem_h );
    ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
    uint8_t* buff = memory_object_get_request_ptr( mem_h );
    ZEPTO_DEBUG_ASSERT( buff != NULL );

    Serial.write((uint8_t) START_OF_PACKET);
    for (i = 0; i < sz; i++) {
        switch (buff[i]) {
            case START_OF_PACKET:
                Serial.write((uint8_t) BYTE_MARKER);
                Serial.write((uint8_t) 0x00);
            break;

            case END_OF_TRANSMISSION:
                Serial.write((uint8_t) BYTE_MARKER);
                Serial.write((uint8_t) 0x02);
            break;

            case BYTE_MARKER:
                Serial.write((uint8_t) BYTE_MARKER);
                Serial.write((uint8_t) 0x03);
            break;

            default:
                if (!Serial.write(buff[i])) {
                    return COMMLAYER_RET_FAILED;
                }
            break;
        }

    }
    Serial.write((uint8_t) END_OF_TRANSMISSION);

    return COMMLAYER_RET_OK;
}
