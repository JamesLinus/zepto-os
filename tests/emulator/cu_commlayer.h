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

#if !defined __SA_COMMLAYER_H__
#define __SA_COMMLAYER_H__

#include <simpleiot/siot_common.h>
#include <simpleiot_hal/siot_mem_mngmt.h>
#include <zepto_mem_mngmt_hal_spec.h>

// RET codes
#define COMMLAYER_RET_FAILED 0
#define COMMLAYER_RET_OK 1
#define COMMLAYER_RET_PENDING 2
#define COMMLAYER_RET_OK_FOR_CU 3
#define COMMLAYER_RET_OK_FOR_SLAVE 4
#define COMMLAYER_RET_OK_FOR_CU_ERROR 5

#define HAL_GET_PACKET_BYTES_IN_PROGRESS 0
#define HAL_GET_PACKET_BYTES_FAILED 1
#define HAL_GET_PACKET_BYTES_DONE 2

#define COMMLAYER_RET_FROM_CENTRAL_UNIT 10
#define COMMLAYER_RET_FROM_DEV 11
#define COMMLAYER_RET_TIMEOUT 12

#define COMMLAYER_RET_FROM_COMMM_STACK 10

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t other_port_num_with_cl; // TODO: in general, it will be more elaborated when more than a single commstack (one for each of multiple slaves) will be used

bool communication_initialize();
void communication_terminate();

uint8_t send_message( MEMORY_HANDLE mem_h );
uint8_t hal_get_packet_bytes( MEMORY_HANDLE mem_h );
uint8_t send_to_commm_stack_as_from_master( MEMORY_HANDLE mem_h, uint16_t terget_id );
uint8_t send_to_commm_stack_as_from_slave( MEMORY_HANDLE mem_h );

uint8_t wait_for_communication_event( unsigned int timeout );
uint8_t try_get_message_within_master( MEMORY_HANDLE mem_h );
//uint8_t send_to_commm_stack_as_from_master( MEMORY_HANDLE mem_h );
uint8_t send_to_commm_stack_as_from_slave( MEMORY_HANDLE mem_h );

#ifdef __cplusplus
}
#endif



#endif // __SA_COMMLAYER_H__