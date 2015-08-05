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

#define MAX_PACKET_SIZE 50


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

#ifdef _MSC_VER
SOCKET sock;
SOCKET sock_accepted;
#else
int sock;
int sock_accepted;
#endif
struct sockaddr_in sa_self, sa_other;
const char* inet_addr_as_string = "127.0.0.1";

uint16_t self_port_num = 7654;
uint16_t other_port_num = 7667;

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

bool _communication_initialize()
{
	//Zero out socket address
	memset(&sa_self, 0, sizeof sa_self);
	memset(&sa_other, 0, sizeof sa_other);

	//create an internet, datagram, socket using UDP
//	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock) /* if socket failed to initialize, exit */
	{
		ZEPTO_DEBUG_PRINTF_1("Error Creating Socket\n");
		return false;
	}

	//The address is ipv4
	sa_other.sin_family = AF_INET;
	sa_self.sin_family = AF_INET;

	//ip_v4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
	sa_self.sin_addr.s_addr = inet_addr( inet_addr_as_string );
	sa_other.sin_addr.s_addr = inet_addr( inet_addr_as_string );

	//sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 7654
	sa_self.sin_port = htons( self_port_num );
	sa_other.sin_port = htons( other_port_num );

	if (-1 == bind(sock, (struct sockaddr *)&sa_self, sizeof(sa_self)))
	{
#ifdef _MSC_VER
		int error = WSAGetLastError();
#else
		int error = errno;
#endif
		ZEPTO_DEBUG_PRINTF_2( "bind sock failed; error %d\n", error );
		CLOSE_SOCKET(sock);
		return false;
	}
/*
#ifdef _MSC_VER
    unsigned long ul = 1;
    ioctlsocket(sock, FIONBIO, &ul);
#else
    fcntl(sock,F_SETFL,O_NONBLOCK);
#endif
	*/
	if(-1 == listen(sock, 10))
    {
      perror("error listen failed");
      CLOSE_SOCKET(sock);
      return false;
    }

	sock_accepted = accept(sock, NULL, NULL);

      if ( 0 > sock_accepted )
      {
        perror("error accept failed");
        CLOSE_SOCKET(sock);
        exit(EXIT_FAILURE);
      }

	  sock = sock_accepted; /*just to keep names*/

#ifdef _MSC_VER
    unsigned long ul = 1;
    ioctlsocket(sock, FIONBIO, &ul);
#else
    fcntl(sock,F_SETFL,O_NONBLOCK);
#endif
	return true;
}

void _communication_terminate()
{
	CLOSE_SOCKET(sock);
}

uint8_t send_message( MEMORY_HANDLE mem_h )
{
/*	uint16_t sz = memory_object_get_request_size( mem_h );
	ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
	uint8_t* buff = memory_object_get_request_ptr( mem_h );
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	int bytes_sent = sendto(sock, (char*)buff, sz, 0, (struct sockaddr*)&sa_other, sizeof sa_other);
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
	return COMMLAYER_RET_OK;*/
	ZEPTO_DEBUG_PRINTF_1( "send_message() called...\n" );

	uint16_t sz = memory_object_get_request_size( mem_h );
	memory_object_request_to_response( mem_h );
	ZEPTO_DEBUG_ASSERT( sz == memory_object_get_response_size( mem_h ) );
	ZEPTO_DEBUG_ASSERT( sz != 0 ); // note: any valid message would have to have at least some bytes for headers, etc, so it cannot be empty
	uint8_t* buff = memory_object_prepend( mem_h, 2 );
	ZEPTO_DEBUG_ASSERT( buff != NULL );
	buff[0] = (uint8_t)sz;
	buff[1] = sz >> 8;
	int bytes_sent = sendto(sock, (char*)buff, sz+2, 0, (struct sockaddr*)&sa_other, sizeof sa_other);
	// do full cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );


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
/*
uint8_t hal_get_packet_bytes( MEMORY_HANDLE mem_h )
{
	// Implementation notes:
	// It is assumed in the current implementation that the system is able to receive a whole packet up to MAX_PACKET_SIZE BYTES first and store it in a buffer of a respective size.
	// Then bytes of the received packet are appended at once to the response referenced by handle.
	//
	// In some other implementations a packet may first be received by parts so that each part is first stored in a relatively small intermediate buffer.
	// In this case this function appends bytes accumulated in the buffer to the response, cleares the buffer, and starts accumulaation over.
	// This process ends when the whole packet is written to the response behind the handle

	// do cleanup
	uint8_t buffer[ MAX_PACKET_SIZE ];
	socklen_t fromlen = sizeof(sa_other);
	uint16_t recsize = recvfrom(sock, (char *)buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&sa_other, &fromlen);

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
			return HAL_GET_PACKET_BYTES_FAILED;
		}
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( recsize && recsize <= MAX_PACKET_SIZE );
		zepto_write_block( mem_h, buffer, recsize );
		return HAL_GET_PACKET_BYTES_DONE;
	}

}
*/
uint8_t try_get_packet( uint8_t* buff, uint16_t sz )
{
	socklen_t fromlen = sizeof(sa_other);
	int recsize = recvfrom(sock, (char *)(buff + buffer_in_pos), sz - buffer_in_pos, 0, (struct sockaddr *)&sa_other, &fromlen);
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

uint8_t try_get_packet_size( uint8_t* buff )
{
	socklen_t fromlen = sizeof(sa_other);
	int recsize = recvfrom(sock, (char *)(buff + buffer_in_pos), 2 - buffer_in_pos, 0, (struct sockaddr *)&sa_other, &fromlen);
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

uint8_t hal_get_packet_bytes( MEMORY_HANDLE mem_h )
{
	// do cleanup
	memory_object_response_to_request( mem_h );
	memory_object_response_to_request( mem_h );
	uint8_t* buff = memory_object_append( mem_h, MAX_PACKET_SIZE );

	buffer_in_pos = 0;
	uint8_t ret;

	do //TODO: add delays or some waiting
	{
		ret = try_get_packet_size( buff );
	}
	while ( ret == COMMLAYER_RET_PENDING );
	if ( ret != COMMLAYER_RET_OK )
		return ret;
	uint16_t sz = buff[1]; sz <<= 8; sz += buff[0];

	buffer_in_pos = 0;
	do //TODO: add delays or some waiting
	{
		ret = try_get_packet( buff, sz );
	}
	while ( ret == COMMLAYER_RET_PENDING );

	memory_object_response_to_request( mem_h );
	memory_object_cut_and_make_response( mem_h, 0, sz );

//	return ret;
	return HAL_GET_PACKET_BYTES_DONE;
}





bool communication_initialize()
{
	return communication_preinitialize() && _communication_initialize();
}

void communication_terminate()
{
	_communication_terminate();
}

//uint8_t wait_for_communication_event( MEMORY_HANDLE mem_h, uint16_t timeout )
uint8_t wait_for_communication_event( unsigned int timeout )
{
	ZEPTO_DEBUG_PRINTF_1( "wait_for_communication_event()... " );
    fd_set rfds;
    struct timeval tv;
    int retval;
	int fd_cnt;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);

    FD_SET(sock, &rfds);
	fd_cnt = (int)(sock + 1);

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
		ZEPTO_DEBUG_ASSERT(0);
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_FAILED\n" );
		return COMMLAYER_RET_FAILED;
	}
    else if (retval)
	{
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_FROM_DEV\n" );
		return COMMLAYER_RET_FROM_DEV;
	}
    else
	{
		ZEPTO_DEBUG_PRINTF_1( "COMMLAYER_RET_TIMEOUT\n" );
        return COMMLAYER_RET_TIMEOUT;
	}
}

uint8_t wait_for_timeout( unsigned int timeout)
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

uint8_t hal_wait_for( waiting_for* wf )
{
	unsigned int timeout = wf->wait_time.high_t;
	timeout <<= 16;
	timeout += wf->wait_time.low_t;
	uint8_t ret_code;
	ZEPTO_DEBUG_ASSERT( wf->wait_legs == 0 ); // not implemented
	ZEPTO_DEBUG_ASSERT( wf->wait_i2c == 0 ); // not implemented
	if ( wf->wait_packet )
	{
		ret_code = wait_for_communication_event( timeout );
		switch ( ret_code )
		{
			case COMMLAYER_RET_FROM_DEV: return WAIT_RESULTED_IN_PACKET; break;
			case COMMLAYER_RET_TIMEOUT: return WAIT_RESULTED_IN_TIMEOUT; break;
			case COMMLAYER_RET_FAILED: return WAIT_RESULTED_IN_FAILURE; break;
			default: return WAIT_RESULTED_IN_FAILURE;
		}
	}
	else
	{
		ret_code = wait_for_timeout( timeout );
		switch ( ret_code )
		{
			case COMMLAYER_RET_TIMEOUT: return WAIT_RESULTED_IN_TIMEOUT; break;
			default: return WAIT_RESULTED_IN_FAILURE;
		}
	}
}

void keep_transmitter_on( bool keep_on )
{
	// TODO: add reasonable implementation
}