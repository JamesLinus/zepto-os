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


//#define MODEL_IN_EFFECT 1
#define MODEL_IN_EFFECT 2


#include "sa-common.h"
#include "sa-uint48.h"
#include "sa-hal-time-provider.h"
#include "sa-commlayer.h"
#include "sa-timer.h"
#include "saoudp_protocol.h"
#include "sasp_protocol.h"
#include "sagdp_protocol.h"
#if MODEL_IN_EFFECT == 1
#include "yoctovm_protocol.h"
#elif MODEL_IN_EFFECT == 2
#include "saccp_protocol.h"
#include "sa_test_plugins.h"
#else
#error #error Unexpected value of MODEL_IN_EFFECT
#endif
#include "test-generator.h"
#include <stdio.h> 


uint8_t pid[ SASP_NONCE_SIZE ];
uint8_t nonce[ SASP_NONCE_SIZE ];









int main_loop()
{
#ifdef ENABLE_COUNTER_SYSTEM
	INIT_COUNTER_SYSTEM
#endif // ENABLE_COUNTER_SYSTEM

		
	printf("STARTING SERVER...\n");
	printf("==================\n\n");

	// TODO: actual key loading, etc
	uint8_t sasp_key[16];
	memcpy( sasp_key, "16-byte fake key", 16 );

	tester_initTestSystem();

	SASP_DATA sasp_data;

	timeout_action tact;
	tact.action = 0;


	// in this preliminary implementation all memory segments are kept separately
	// All memory objects are listed below
	// TODO: revise memory management
//	uint8_t timer_val = 0;
//	uint16_t wake_time = 0;
	// TODO: revise time/timer management

//	DefaultTestingPluginConfig pl_conf;
//	DefaultTestingPluginState pl_state;

	uint8_t ret_code;

	// test setup values
	bool wait_for_incoming_chain_with_timer = 0;
	uint16_t wake_time_to_start_new_chain = 0;

	uint8_t wait_to_continue_processing = 0;
	uint16_t wake_time_continue_processing = 0;

	// do necessary initialization
	SASP_initAtLifeStart( &sasp_data ); // TODO: replace by more extensive restore-from-backup-etc
	SAGDP_DATA sagdp_data;
	sagdp_init( &sagdp_data );
	void zepto_vm_init();


	printf("\nAwaiting client connection... \n" );
	if (!communication_initialize())
		return -1;

	printf("Client connected.\n");

	// MAIN LOOP
	for (;;)
	{
getmsg:
#if MODEL_IN_EFFECT == 1
		if ( wait_to_continue_processing && getTime() >= wake_time_continue_processing )
		{
printf( "Processing continued...\n" );
			INCREMENT_COUNTER( 98, "MAIN LOOP, continuing processing" );
			wait_to_continue_processing = 0;
#ifdef USED_AS_MASTER
			ret_code = master_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#else
			ret_code = slave_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#endif
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			goto entry;
			break;
		}
#elif MODEL_IN_EFFECT == 2
#else
#error #error Unexpected value of MODEL_IN_EFFECT
#endif

		// 1. Get message from comm peer
		ret_code = tryGetMessage( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		assert( ret_code != COMMLAYER_RET_OK || ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ) != 0 );
		INCREMENT_COUNTER_IF( 91, "MAIN LOOP, packet received [1]", ret_code == COMMLAYER_RET_OK );
		while ( ret_code == COMMLAYER_RET_PENDING )
		{
			//waitForTimeQuantum();
			justWaitMSec( 200 );
#if MODEL_IN_EFFECT == 1
			if ( wait_to_continue_processing && getTime() >= wake_time_continue_processing )
			{
printf( "Processing continued...\n" );
				INCREMENT_COUNTER( 98, "MAIN LOOP, continuing processing" );
				wait_to_continue_processing = 0;
#ifdef USED_AS_MASTER
				ret_code = master_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#else
				ret_code = slave_process_continue( MEMORY_HANDLE_MAIN_LOOP );
#endif
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto entry;
				break;
			}
#elif MODEL_IN_EFFECT == 2
#else
#error #error Unexpected value of MODEL_IN_EFFECT
#endif
//			if ( timer_val && getTime() >= wake_time )
//			if ( tact.action )
			if ( sagdp_data.event_type ) //TODO: temporary solution
			{
				printf( "no reply received; the last message (if any) will be resent by timer\n" );
				sa_get_time( &(tact.tv) ); tact.action = 0;
				ret_code = handler_sagdp_timer( &tact, NULL, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
				if ( ret_code == SAGDP_RET_OK )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					goto trygetmsg;
				}
				else if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE, &sasp_data );
					assert( ret_code == SASP_RET_NONCE );
					sa_get_time( &(tact.tv) ); tact.action = 0;
					ret_code = handler_sagdp_timer( &tact, nonce, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
		printf( "ret_code = %d\n", ret_code );
					assert( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_OK );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					goto saspsend;
					break;
				}
				else
				{
					printf( "ret_code = %d\n", ret_code );
					assert( 0 );
				}
			}
			else if ( wait_for_incoming_chain_with_timer && getTime() >= wake_time_to_start_new_chain )
			{
				wait_for_incoming_chain_with_timer = false;
#if MODEL_IN_EFFECT == 1
				ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP );
#elif MODEL_IN_EFFECT == 2
#else
#error #error Unexpected value of MODEL_IN_EFFECT
#endif
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto alt_entry;
				break;
			}
trygetmsg:
			ret_code = tryGetMessage( MEMORY_HANDLE_MAIN_LOOP );
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER_IF( 92, "MAIN LOOP, packet received [2]", ret_code == COMMLAYER_RET_OK );
			assert( ret_code != COMMLAYER_RET_OK || ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ) != 0 );
		}
		if ( ret_code != COMMLAYER_RET_OK )
		{
			printf("\n\nWAITING FOR ESTABLISHING COMMUNICATION WITH SERVER...\n\n");
			if (!communication_initialize()) // regardles of errors... quick and dirty solution so far
				return -1;
			goto getmsg;
		}
		printf("Message from client received\n");
		printf( "ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );


		// 2.1. Pass to SAoUDP
		ret_code = handler_saoudp_receive( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

		switch ( ret_code )
		{
			case SAOUDP_RET_OK:
			{
				// regular processing will be done below in the next block
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}


		// 2.2. Pass to SASP
		ret_code = handler_sasp_receive( sasp_key, pid, MEMORY_HANDLE_MAIN_LOOP, &sasp_data );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SASP1:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SASP_RET_IGNORE:
			{
				printf( "BAD MESSAGE_RECEIVED\n" );
				goto getmsg;
				break;
			}
			case SASP_RET_TO_LOWER_ERROR:
			{
				goto saoudp_send;
				break;
			}
			case SASP_RET_TO_HIGHER_NEW:
			{
				// regular processing will be done below in the next block
				break;
			}
			case SASP_RET_TO_HIGHER_LAST_SEND_FAILED: // as a result of error-old-nonce
			{
				printf( "NONCE_LAST_SENT has been reset; the last message (if any) will be resent\n" );
				sa_get_time( &(tact.tv) ); tact.action = 0;
				ret_code = handler_sagdp_receive_request_resend_lsp( &tact, NULL, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
				if ( ret_code == SAGDP_RET_TO_LOWER_NONE )
				{
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					continue;
				}
				if ( ret_code == SAGDP_RET_NEED_NONCE )
				{
					ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE, &sasp_data );
					assert( ret_code == SASP_RET_NONCE );
					sa_get_time( &(tact.tv) ); tact.action = 0;
					ret_code = handler_sagdp_receive_request_resend_lsp( &tact, nonce, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
					assert( ret_code != SAGDP_RET_NEED_NONCE && ret_code != SAGDP_RET_TO_LOWER_NONE );
				}
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

		// 3. pass to SAGDP a new packet
		sa_get_time( &(tact.tv) ); tact.action = 0;
		ret_code = handler_sagdp_receive_up( &tact, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
		if ( ret_code == SAGDP_RET_START_OVER_FIRST_RECEIVED )
		{
			sagdp_init( &sagdp_data );
			// TODO: do remaining reinitialization
			sa_get_time( &(tact.tv) ); tact.action = 0;
			ret_code = handler_sagdp_receive_up( &tact, NULL, pid, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
			assert( ret_code != SAGDP_RET_START_OVER_FIRST_RECEIVED );
		}
		else if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE, &sasp_data );
			assert( ret_code == SASP_RET_NONCE );
			sa_get_time( &(tact.tv) ); tact.action = 0;
			ret_code = handler_sagdp_receive_up( &tact, nonce, pid, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SAGDP1: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
#ifdef USED_AS_MASTER
			case SAGDP_RET_OK:
			{
				printf( "master received unexpected packet. ignored\n" );
				goto getmsg;
				break;
			}
#else
			case SAGDP_RET_SYS_CORRUPTED:
			{
				// TODO: reinitialize all
				sagdp_init( &sagdp_data );
//				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}
#endif
			case SAGDP_RET_TO_HIGHER:
			{
				// regular processing will be done below in the next block
				break;
			}
			case SAGDP_RET_TO_LOWER_REPEATED:
			{
				goto saspsend;
			}
			case SAGDP_RET_OK:
			{
				goto getmsg;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

#if MODEL_IN_EFFECT == 1
processcmd:
		// 4. Process received command (yoctovm)
		ret_code = slave_process( &wait_to_continue_processing, MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4*/ );
/*		if ( ret_code == YOCTOVM_RESET_STACK )
		{
			sagdp_init( &sagdp_data );
			printf( "slave_process(): ret_code = YOCTOVM_RESET_STACK\n" );
			// TODO: reinit the rest of stack (where applicable)
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
		}*/
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
entry:
		wait_for_incoming_chain_with_timer = false;
		if ( ret_code == YOCTOVM_WAIT_TO_CONTINUE )
			printf( "YOCTO:  ret: %d; waiting to continue ...\n", ret_code );
		else
			printf( "YOCTO:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case YOCTOVM_PASS_LOWER:
			{
				 // test generation: sometimes slave can start a new chain at not in-chain reason (although in this case it won't be accepted by Master)
//				bool restart_chain = tester_get_rand_val() % 8 == 0;
				bool restart_chain = false;
				if ( restart_chain )
				{
					sagdp_init( &sagdp_data );
					ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4*/ );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					assert( ret_code == YOCTOVM_PASS_LOWER );
				}
				// regular processing will be done below in the next block
				break;
			}
			case YOCTOVM_PASS_LOWER_THEN_IDLE:
			{
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + tester_get_rand_val() % 8;
				wait_for_incoming_chain_with_timer = true;
				break;
			}
			case YOCTOVM_FAILED:
				sagdp_init( &sagdp_data );
				// NOTE: no 'break' is here as the rest is the same as for YOCTOVM_OK
			case YOCTOVM_OK:
			{
				// here, in general, two main options are present: 
				// (1) to start a new chain immediately, or
				// (2) to wait, during certain period of time, for an incoming chain, and then, if no packet is received, to start a new chain
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				if ( start_now )
				{
					PRINTF( "   ===  YOCTOVM_OK, forced chain restart  ===\n" );
					ret_code = master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4*/ );
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					assert( ret_code == YOCTOVM_PASS_LOWER );
					// one more trick: wait for some time to ensure that master will start its own chain, and then send "our own" chain start
/*					bool mutual = tester_get_rand_val() % 5 == 0;
					if ( mutual )
						justWait( 4 );*/
				}
				else
				{
					PRINTF( "   ===  YOCTOVM_OK, delayed chain restart  ===\n" );
					wake_time_to_start_new_chain = getTime() + tester_get_rand_val() % 8;
					wait_for_incoming_chain_with_timer = true;
					goto getmsg;
				}
				break;
			}
			case YOCTOVM_WAIT_TO_CONTINUE:
			{
				if ( wait_to_continue_processing == 0 ) wait_to_continue_processing = 1;
				wake_time_continue_processing = getTime() + wait_to_continue_processing;
printf( "Processing in progress... (period = %d, time = %d)\n", wait_to_continue_processing, wake_time_continue_processing );
				goto getmsg;
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

#elif MODEL_IN_EFFECT == 2

processcmd:
		// 4. Process received command (yoctovm)
		ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP, /*sasp_nonce_type chain_id*/NULL ); // slave_process( &wait_to_continue_processing, MEMORY_HANDLE_MAIN_LOOP );
/*		if ( ret_code == YOCTOVM_RESET_STACK )
		{
			sagdp_init( &sagdp_data );
			printf( "slave_process(): ret_code = YOCTOVM_RESET_STACK\n" );
			// TODO: reinit the rest of stack (where applicable)
			ret_code = master_start( sizeInOut, rwBuff, rwBuff + BUF_SIZE / 4 );
		}*/
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
entry:
		wait_for_incoming_chain_with_timer = false;
/*		if ( ret_code == YOCTOVM_WAIT_TO_CONTINUE )
			printf( "YOCTO:  ret: %d; waiting to continue ...\n", ret_code );
		else
			printf( "YOCTO:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );*/

		switch ( ret_code )
		{
			case SACCP_RET_PASS_LOWER:
			{
				 // test generation: sometimes slave can start a new chain at not in-chain reason (although in this case it won't be accepted by Master)
//				bool restart_chain = tester_get_rand_val() % 8 == 0;
				bool restart_chain = false;
				if ( restart_chain )
				{
					sagdp_init( &sagdp_data );
					ret_code = handler_saccp_receive( MEMORY_HANDLE_MAIN_LOOP, /*sasp_nonce_type chain_id*/NULL ); // master_start( MEMORY_HANDLE_MAIN_LOOP/*, BUF_SIZE / 4*/ )
					zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
					assert( ret_code == SACCP_RET_PASS_LOWER );
				}
				// regular processing will be done below in the next block
				break;
			}
/*			case YOCTOVM_PASS_LOWER_THEN_IDLE:
			{
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + tester_get_rand_val() % 8;
				wait_for_incoming_chain_with_timer = true;
				break;
			}*/
		}
#else
#error #error Unexpected value of MODEL_IN_EFFECT
#endif
			
			
		// 5. SAGDP
alt_entry:
//		uint8_t timer_val;
		sa_get_time( &(tact.tv) ); tact.action = 0;
		ret_code = handler_sagdp_receive_hlp( &tact, NULL, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
		if ( ret_code == SAGDP_RET_NEED_NONCE )
		{
			ret_code = handler_sasp_get_packet_id( nonce, SASP_NONCE_SIZE, &sasp_data );
			assert( ret_code == SASP_RET_NONCE );
			sa_get_time( &(tact.tv) ); tact.action = 0;
			ret_code = handler_sagdp_receive_hlp( &tact, nonce, MEMORY_HANDLE_MAIN_LOOP, &sagdp_data );
			assert( ret_code != SAGDP_RET_NEED_NONCE );
		}
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SAGDP2: ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SAGDP_RET_TO_LOWER_NEW:
			{
				// regular processing will be done below in the next block
				// set timer
				break;
			}
			case SAGDP_RET_OK: // TODO: is it possible here?
			{
				goto getmsg;
				break;
			}
			case SAGDP_RET_TO_LOWER_REPEATED: // TODO: is it possible here?
			{
				goto getmsg;
				break;
			}
			case SAGDP_RET_SYS_CORRUPTED: // TODO: is it possible here?
			{
				// TODO: process reset
				sagdp_init( &sagdp_data );
//				bool start_now = tester_get_rand_val() % 3;
				bool start_now = true;
				wake_time_to_start_new_chain = start_now ? getTime() : getTime() + tester_get_rand_val() % 8;
				wait_for_incoming_chain_with_timer = true;
				zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
				goto saspsend;
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

		// SASP
saspsend:
		ret_code = handler_sasp_send( sasp_key, nonce, MEMORY_HANDLE_MAIN_LOOP, &sasp_data );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
		printf( "SASP2:  ret: %d; rq_size: %d, rsp_size: %d\n", ret_code, ugly_hook_get_request_size( MEMORY_HANDLE_MAIN_LOOP ), ugly_hook_get_response_size( MEMORY_HANDLE_MAIN_LOOP ) );

		switch ( ret_code )
		{
			case SASP_RET_TO_LOWER_REGULAR:
			{
				// regular processing will be done below in the next block
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}


		// Pass to SAoUDP
saoudp_send:
		ret_code = handler_saoudp_send( MEMORY_HANDLE_MAIN_LOOP );
		zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );

		switch ( ret_code )
		{
			case SAOUDP_RET_OK:
			{
				// regular processing will be done below in the next block
				break;
			}
			default:
			{
				// unexpected ret_code
				printf( "Unexpected ret_code %d\n", ret_code );
				assert( 0 );
				break;
			}
		}

sendmsg:
			ret_code = sendMessage( MEMORY_HANDLE_MAIN_LOOP );
			if (ret_code != COMMLAYER_RET_OK )
			{
				return -1;
			}
			zepto_response_to_request( MEMORY_HANDLE_MAIN_LOOP );
			INCREMENT_COUNTER( 90, "MAIN LOOP, packet sent" );
			printf("\nMessage replied to client\n");

	}

	return 0;
}

int main(int argc, char *argv[])
{
	zepto_mem_man_init_memory_management();

	return main_loop();
}