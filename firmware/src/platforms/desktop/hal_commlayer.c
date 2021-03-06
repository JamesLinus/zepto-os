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

#include <zepto_mem_mngmt_hal_spec.h>
#include <simpleiot_hal/hal_commlayer.h>
#include <simpleiot_hal/hal_waiting.h>
#include <stdio.h>

#define MAX_PACKET_SIZE 128

#ifdef USED_AS_RETRANSMITTER
#define BUS_COUNT 2
#else
#define BUS_COUNT 1
#endif


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define CLOSE_SOCKET( x ) closesocket( x )

#else // _MSC_VER

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close() for socket
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define CLOSE_SOCKET( x ) close( x )

#endif // _MSC_VER

#ifdef MESH_TEST
#ifdef SA_RETRANSMITTER

#ifdef _MSC_VER
SOCKET sock;
SOCKET sock_accepted;
SOCKET sock2;
SOCKET sock_accepted2;
#else
int sock;
int sock_accepted;
int sock2;
int sock_accepted2;
#endif
struct sockaddr_in sa_self, sa_other;
const char* inet_addr_as_string = "127.0.0.1";
uint16_t self_port_num = 7654;
uint16_t other_port_num = 7667;

struct sockaddr_in sa_self2, sa_other2;
uint16_t self_port_num2 = 7767;
uint16_t other_port_num2 = 7754;

#else // terminal device
#ifdef _MSC_VER
SOCKET sock;
SOCKET sock_accepted;
#else
int sock;
int sock_accepted;
#endif
struct sockaddr_in sa_self, sa_other;
const char* inet_addr_as_string = "127.0.0.1";
uint16_t self_port_num = 7754;
uint16_t other_port_num = 7767;
#endif
#else
#ifdef _MSC_VER
SOCKET sock[ BUS_COUNT ];
SOCKET sock_accepted[ BUS_COUNT ];
#else
int sock[ BUS_COUNT ];
int sock_accepted[ BUS_COUNT ];
#endif
struct sockaddr_in sa_other[ BUS_COUNT ];

const char* inet_addr_as_string = "127.0.0.1";
#ifdef USED_AS_RETRANSMITTER
uint16_t other_port_num = 7654;
#else
uint16_t other_port_num = 7655;
#endif
#endif

uint16_t buffer_in_pos;



bool communication_preinitialize()
{
#ifdef _MSC_VER
	// do Windows magic
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		ZEPTO_DEBUG_PRINTF_2("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
	return true;
#else
	return true;
#endif
}

#if (defined MESH_TEST) && (defined SA_RETRANSMITTER)
bool _communication_initialize_2()
{
	//Zero out socket address
	memset(&sa_self2, 0, sizeof sa_self2);
	memset(&sa_other2, 0, sizeof sa_other2);

	//create an internet, datagram, socket using UDP
	sock2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock2) /* if socket failed to initialize, exit */
	{
		ZEPTO_DEBUG_PRINTF_1("Error Creating Socket\n");
		return false;
	}

	//The address is ipv4
	sa_other2.sin_family = AF_INET;
	sa_self2.sin_family = AF_INET;

	//ip_v4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
	sa_self2.sin_addr.s_addr = inet_addr( inet_addr_as_string );
	sa_other2.sin_addr.s_addr = inet_addr( inet_addr_as_string );

	//sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 7654
	sa_self2.sin_port = htons( self_port_num2 );
	sa_other2.sin_port = htons( other_port_num2 );

	if (-1 == bind(sock2, (struct sockaddr *)&sa_self2, sizeof(sa_self2)))
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
#else
		int error = errno;
#endif
		ZEPTO_DEBUG_PRINTF_2( "bind sock failed; error %d\n", error );
		CLOSE_SOCKET(sock2);
		return false;
	}

	if (-1 == connect(sock2, (struct sockaddr *)&sa_other2, sizeof(sa_other2)))
		{
		  perror("connect failed");
			CLOSE_SOCKET(sock2);
		  return false;
		}
	return true;
}

#endif // (defined MESH_TEST) && (defined SA_RETRANSMITTER)


bool _communication_initialize_for_each_bus( uint8_t bus_id )
{
	ZEPTO_DEBUG_ASSERT( bus_id < BUS_COUNT );
	//Zero out socket address
	memset(&(sa_other[ bus_id ]), 0, sizeof(struct sockaddr_in));

	//create an internet, datagram, socket using UDP
	sock[ bus_id ] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock[ bus_id ]) /* if socket failed to initialize, exit */
	{
		ZEPTO_DEBUG_PRINTF_1("Error Creating Socket\n");
		return false;
	}

	//The address is ipv4
	sa_other[ bus_id ].sin_family = AF_INET;

	//ip_v4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
	sa_other[ bus_id ].sin_addr.s_addr = inet_addr( inet_addr_as_string );

	//sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 7654
	sa_other[ bus_id ].sin_port = htons( other_port_num + bus_id );

	if (-1 == connect(sock[ bus_id ], (struct sockaddr *)&(sa_other[ bus_id ]), sizeof(struct sockaddr_in)))
		{
		  perror("connect failed");
			CLOSE_SOCKET(sock[ bus_id ]);
		  return false;
		}
	return true;
}

bool _communication_initialize()
{
	uint8_t bus_id = 0;
	for ( bus_id=0; bus_id<BUS_COUNT; bus_id++ )
	{
		if ( !_communication_initialize_for_each_bus( bus_id ) )
			return false;
	}
	return true;
}

void _communication_terminate_for_bus( uint8_t bus_id )
{
	CLOSE_SOCKET(sock[bus_id]);
#if (defined MESH_TEST) && (defined SA_RETRANSMITTER)
	CLOSE_SOCKET(sock2);
#endif
}

void _communication_terminate()
{
	uint8_t bus_id = 0;
	for ( bus_id=0; bus_id<BUS_COUNT; bus_id++ )
		_communication_terminate_for_bus( bus_id );
#if (defined MESH_TEST) && (defined SA_RETRANSMITTER)
#error not implemented
	CLOSE_SOCKET(sock2);
#endif
}

uint8_t internal_send_packet( MEMORY_HANDLE mem_h, int _sock, struct sockaddr* _sa_other )
{
	ZEPTO_DEBUG_PRINTF_1( "send_message() called...\n" );

	uint16_t sz = memory_object_get_request_size( mem_h );
	memory_object_request_to_response( mem_h );
	ZEPTO_DEBUG_ASSERT( sz == memory_object_get_response_size( mem_h ) );
	ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
	uint8_t* buff = memory_object_prepend( mem_h, 3 );
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	buff[0] = (uint8_t)(sz+1);
	buff[1] = (sz+1) >> 8;
	buff[2] = 0; // "regular" packet within testing system. NOTE: since AIR is used, it won't be received by an ultimate recipient
	int bytes_sent = sendto(_sock, (char*)buff, sz+3, 0, _sa_other, sizeof (struct sockaddr) );

	// restore data behind handle
	zepto_response_to_request( mem_h );
	parser_obj po, po1;
	zepto_parser_init( &po, mem_h );
	zepto_parse_skip_block( &po, 3 );
	zepto_parser_init_by_parser( &po1, &po );
	zepto_parse_skip_block( &po1, zepto_parsing_remaining_bytes( &po ) );
	zepto_convert_part_of_request_to_response( mem_h, &po, &po1 );
	zepto_response_to_request( mem_h );


	if (bytes_sent < 0)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		ZEPTO_DEBUG_PRINTF_2( "Error %d sending packet\n", error );
#else
		ZEPTO_DEBUG_PRINTF_2("Error sending packet: %s\n", strerror(errno));
#endif
		return COMMLAYER_RET_FAILED;
	}
#ifdef _MSC_VER
	ZEPTO_DEBUG_PRINTF_4( "[%d] message sent; mem_h = %d, size = %d\n", GetTickCount(), mem_h, sz );
#else
	ZEPTO_DEBUG_PRINTF_3( "[--] message sent; mem_h = %d, size = %d\n", mem_h, sz );
#endif
	return COMMLAYER_RET_OK;
}

#ifdef MESH_TEST
uint8_t hal_send_packet( MEMORY_HANDLE mem_h, uint8_t bus_id, uint8_t intrabus_id )
{
	uint8_t ret_code;
#ifdef SA_RETRANSMITTER
	if ( bus_id == 0 )
		ret_code = internal_send_packet( mem_h, sock, (struct sockaddr *)(&sa_other) );
	else
	{
		ZEPTO_DEBUG_ASSERT( bus_id == 1 );
		ret_code = internal_send_packet( mem_h, sock2, (struct sockaddr *)(&sa_other2) );
	}
#else
	ZEPTO_DEBUG_ASSERT( bus_id == 0 );
	ret_code = internal_send_packet( mem_h, sock, (struct sockaddr *)(&sa_other) );
#endif
	// do full cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );

	return ret_code;
}
#else
uint8_t send_message( MEMORY_HANDLE mem_h, uint16_t bus_id )
{
	uint8_t ret_code;
	ZEPTO_DEBUG_ASSERT( bus_id < BUS_COUNT );
	ret_code = internal_send_packet( mem_h, sock[ bus_id ], (struct sockaddr *)(&(sa_other[ bus_id ])) );
	// do full cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );

	return ret_code;
}
#endif


uint8_t try_get_packet( uint8_t* buff, uint16_t sz, int _sock, struct sockaddr* _sa_other )
{
	socklen_t fromlen = sizeof(struct sockaddr_in);
	int recsize = recvfrom(_sock, (char *)(buff + buffer_in_pos), sz - buffer_in_pos, 0, _sa_other, &fromlen);
	if (recsize < 0)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		if ( error == WSAEWOULDBLOCK )
#else
		int error = errno;
		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		{
			return COMMLAYER_RET_PENDING;
		}
		else
		{
			ZEPTO_DEBUG_PRINTF_2( "unexpected error %d received while getting message\n", error );
			return COMMLAYER_RET_FAILED;
		}
	}
	else
	{
		buffer_in_pos += recsize;
		if ( buffer_in_pos < sz )
		{
			return COMMLAYER_RET_PENDING;
		}
		return COMMLAYER_RET_OK;
	}

}

uint8_t try_get_packet_size( uint8_t* buff, int _sock, struct sockaddr* _sa_other )
{
	socklen_t fromlen = sizeof(struct sockaddr_in);
	int recsize = recvfrom(_sock, (char *)(buff + buffer_in_pos), 2 - buffer_in_pos, 0, _sa_other, &fromlen);
	if (recsize < 0)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		if ( error == WSAEWOULDBLOCK )
#else
		int error = errno;
		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		{
			return COMMLAYER_RET_PENDING;
		}
		else
		{
			ZEPTO_DEBUG_PRINTF_2( "unexpected error %d received while getting message\n", error );
			return COMMLAYER_RET_FAILED;
		}
	}
	else
	{
		buffer_in_pos += recsize;
		if ( buffer_in_pos < 2 )
		{
			return COMMLAYER_RET_PENDING;
		}
		return COMMLAYER_RET_OK;
	}

}

uint8_t internal_get_packet_bytes( MEMORY_HANDLE mem_h, int _sock, struct sockaddr* _sa_other )
{
	// do cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );
	uint8_t* buff = memory_object_append( mem_h, MAX_PACKET_SIZE );

	buffer_in_pos = 0;
	uint8_t ret;

	do //TODO: add delays or some waiting
	{
		ret = try_get_packet_size( buff, _sock, _sa_other );
	}
	while ( ret == COMMLAYER_RET_PENDING );
	if ( ret != COMMLAYER_RET_OK )
		return HAL_GET_PACKET_BYTES_FAILED;
	uint16_t sz = buff[1]; sz <<= 8; sz += buff[0];

	buffer_in_pos = 0;
	do //TODO: add delays or some waiting
	{
		ret = try_get_packet( buff, sz, _sock, _sa_other );
	}
	while ( ret == COMMLAYER_RET_PENDING );

	memory_object_response_to_request( mem_h );
	memory_object_cut_and_make_response( mem_h, 0, sz );

	return ret == COMMLAYER_RET_OK ? HAL_GET_PACKET_BYTES_DONE : HAL_GET_PACKET_BYTES_FAILED;
}

#ifdef MESH_TEST
	uint8_t bus_id_in = 0;
#endif


uint8_t hal_get_packet_bytes( MEMORY_HANDLE mem_h, uint16_t bus_id )
{
#ifdef MESH_TEST
#ifdef USE_TIME_MASTER // NOTE: code with USE_TIME_MASTER defined is intended for testing purposes only on 'desktop' platform and should not be taken as a sample for any other platform
#error not implemented
#endif // USE_TIME_MASTER
#ifdef SA_RETRANSMITTER
	if ( bus_id_in == 0 )
		return internal_get_packet_bytes( mem_h, sock, (struct sockaddr *)(&sa_other) );
	else
	{
		ZEPTO_DEBUG_ASSERT( bus_id_in == 1 );
		return internal_get_packet_bytes( mem_h, sock2, (struct sockaddr *)(&sa_other2) );
	}
#else
	ZEPTO_DEBUG_ASSERT( bus_id_in == 0 );
	return internal_get_packet_bytes( mem_h, sock, (struct sockaddr *)(&sa_other) );
#endif

#else // MESH_TEST
	ZEPTO_DEBUG_ASSERT( bus_id < BUS_COUNT );
	return internal_get_packet_bytes( mem_h, sock[ bus_id ], (struct sockaddr *)(&(sa_other[bus_id])) );
#endif
}



typedef struct _DEVICE_POSITION
{
	float x;
	float y;
	float z;
} DEVICE_POSITION;

typedef struct _TEST_DATA
{
	DEVICE_POSITION pos;
} TEST_DATA;

#if !defined USED_AS_MASTER
#ifdef USED_AS_RETRANSMITTER
TEST_DATA test_data = { 1.0, 0, 0};
#else
TEST_DATA test_data = { 2.0, 0, 0};
#endif
#endif


void report_coordinates_to_air( uint8_t bus_id )
{
	uint8_t buff[ 3 + sizeof(TEST_DATA) ];
	uint16_t sz = sizeof(TEST_DATA);
	buff[0] = (uint8_t)(sz+1);
	buff[1] = (sz+1) >> 8;
	buff[2] = 1; // "service" packet within testing system
	uint16_t pos = 3;
	ZEPTO_MEMCPY( buff + pos, &(test_data.pos.x), sizeof(test_data.pos.x) ); // NOTE: we assume here local use and we do not care (so far) about endianness
	pos += sizeof(test_data.pos.x);
	ZEPTO_MEMCPY( buff + pos, &(test_data.pos.y), sizeof(test_data.pos.y) );
	pos += sizeof(test_data.pos.x);
	ZEPTO_MEMCPY( buff + pos, &(test_data.pos.z), sizeof(test_data.pos.z) );
	pos += sizeof(test_data.pos.x);
	int bytes_sent = sendto( sock[bus_id], (char*)buff, sz+3, 0, (struct sockaddr *)(&(sa_other[bus_id])), sizeof (struct sockaddr) );
}

void report_coordinates()
{
	uint8_t bus_id = 0;
	for ( bus_id=0; bus_id<BUS_COUNT; bus_id++ )
		report_coordinates_to_air( bus_id );
}

bool communication_initialize()
{
	bool ret;
#if (defined MESH_TEST) && (defined SA_RETRANSMITTER)
	ret = communication_preinitialize() && _communication_initialize() && _communication_initialize_2();
#else
	ret = communication_preinitialize() && _communication_initialize();
#endif
	if ( ret )
	{
		report_coordinates();
	}
	return ret;
}

void communication_terminate()
{
	_communication_terminate();
}

#if (defined MESH_TEST) && (defined SA_RETRANSMITTER)
uint8_t hal_get_busid_of_last_packet()
{
	return bus_id_in;
}
#endif

uint8_t internal_wait_for_communication_event( unsigned int timeout, uint16_t* using_bus_id )
{
	ZEPTO_DEBUG_PRINTF_1( "internal_wait_for_communication_event()... " );
    fd_set rfds;
    struct timeval tv;
    int retval;
	int fd_cnt = 0;

	*using_bus_id = 0xFFFF;

    /* Watch stdin (fd 0) to see when it has input. *///, uint16_t bus_id
    FD_ZERO(&rfds);

#ifdef MESH_TEST
#error not supported
#ifdef SA_RETRANSMITTER
    FD_SET(sock, &rfds);
    FD_SET(sock2, &rfds);
	fd_cnt = (int)(sock > sock2 ? sock + 1 : sock2 + 1);
#else
    FD_SET(sock, &rfds);
	fd_cnt = (int)(sock + 1);
#endif
#else
	uint8_t bus_id = 0;
	for ( bus_id=0; bus_id<BUS_COUNT; bus_id++ )
	{
		FD_SET(sock[ bus_id ], &rfds);
		if ( fd_cnt < (int)(sock[ bus_id ]) )
			fd_cnt = (int)(sock[ bus_id ]);
	}
	fd_cnt++; // greater than greatest
#endif

    /* Wait */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = ((long)timeout % 1000) * 1000;

    retval = select(fd_cnt, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
//		if ( error == WSAEWOULDBLOCK )
		ZEPTO_DEBUG_PRINTF_2( "error %d\n", error );
#else
        perror("select()");
//		int error = errno;
//		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
        return COMMLAYER_RET_TIMEOUT;
		ZEPTO_DEBUG_ASSERT(0);
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_FAILED\n" );
		return COMMLAYER_RET_FAILED;
	}
    else if (retval)
	{
#ifdef MESH_TEST
#error not supported
#ifdef SA_RETRANSMITTER
		// so far, just resent to the other direction
		if ( FD_ISSET(sock, &rfds) )
		{
			ZEPTO_DEBUG_PRINTF_1( "Retransmitter: packet has come from MASTER\n" );
			bus_id_in = 0;
/*			MEMORY_HANDLE mem_h = 0;
			hal_get_packet_bytes( mem_h );
			zepto_response_to_request( mem_h );
			hal_send_packet( mem_h, 1, 0 );*/
		}
		else
		{
			ZEPTO_DEBUG_ASSERT( FD_ISSET(sock2, &rfds) );
			ZEPTO_DEBUG_PRINTF_1( "Retransmitter: packet has come from SLAVE\n" );
			bus_id_in = 1;
/*			MEMORY_HANDLE mem_h = 0;
			hal_get_packet_bytes( mem_h );
			zepto_response_to_request( mem_h );
			hal_send_packet( mem_h, 0, 0 );*/
		}
#else
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_FROM_DEV\n" );
#endif
#else
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_FROM_DEV\n" );
#endif
		for ( bus_id=0; bus_id<BUS_COUNT; bus_id++ )
			if ( FD_ISSET(sock[ bus_id ], &rfds) )
				*using_bus_id = bus_id;
		ZEPTO_DEBUG_ASSERT( *using_bus_id != 0xFFFF );
		return COMMLAYER_RET_FROM_DEV;
	}
    else
	{
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_TIMEOUT\n" );
        return COMMLAYER_RET_TIMEOUT;
	}
}

uint8_t internal_wait_for_timeout( unsigned int timeout)
{
    struct timeval tv;
    int retval;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = ((long)timeout % 1000) * 1000;

    retval = select(0, NULL, NULL, NULL, &tv);

    if (retval == -1)
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
		ZEPTO_DEBUG_PRINTF_2( "error %d\n", error );
#else
        perror("select()");
//		int error = errno;
//		if ( error == EAGAIN || error == EWOULDBLOCK )
#endif
		ZEPTO_DEBUG_ASSERT(0);
		return COMMLAYER_RET_FAILED;
	}
    else
	{
        return COMMLAYER_RET_TIMEOUT;
	}
}

uint8_t hal_wait_for( waiting_for* wf, uint16_t* bus_id )
{
	unsigned int timeout = wf->wait_time.high_t;
	timeout <<= 16;
	timeout += wf->wait_time.low_t;
	uint8_t ret_code;
	ZEPTO_DEBUG_ASSERT( wf->wait_legs == 0 ); // not implemented
	ZEPTO_DEBUG_ASSERT( wf->wait_i2c == 0 ); // not implemented
//	ZEPTO_DEBUG_PRINTF_3( ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> wait requested: time = %08x (%d) ms <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", timeout, timeout );
//	int now = GetTickCount();
	if ( wf->wait_packet )
	{
		ret_code = internal_wait_for_communication_event( timeout, bus_id );
//		ZEPTO_DEBUG_PRINTF_2( ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> actual wait time = %d ms <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", GetTickCount() - now );
		switch ( ret_code )
		{
			case COMMLAYER_RET_FROM_DEV: ret_code = WAIT_RESULTED_IN_PACKET; break;
			case COMMLAYER_RET_TIMEOUT: ret_code = WAIT_RESULTED_IN_TIMEOUT; break;
			case COMMLAYER_RET_FAILED: ret_code = WAIT_RESULTED_IN_FAILURE; break;
			default: ret_code = WAIT_RESULTED_IN_FAILURE; break;
		}
	}
	else
	{
		ret_code = internal_wait_for_timeout( timeout );
//		ZEPTO_DEBUG_PRINTF_2( ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> actual wait time = %d ms <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", GetTickCount() - now );
		switch ( ret_code )
		{
			case COMMLAYER_RET_TIMEOUT: ret_code = WAIT_RESULTED_IN_TIMEOUT; break;
			default: ret_code = WAIT_RESULTED_IN_FAILURE; break;
		}
	}

	return ret_code;
}

void keep_transmitter_on( bool keep_on )
{
	// TODO: add reasonable implementation
}
