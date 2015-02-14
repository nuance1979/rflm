/*
 * Socket.cc --
 *     Wrapper class for Berkeley sockets
 *
 * Yi Su <suy@jhu.edu>
 *     based on examples given by Rob Tougher <rtougher@yahoo.com>
 *     in "Linux Socket Programming In C++", Issue 74 of Linux Gazette, 
 *     January 2002 <http://linuxgazette.net/issue74/tougher.html>
 */

#ifdef PRE_ISO_CXX
#include <iostream.h>
#include <string.h>
#else
#include <iostream>
#include <cstring>
using namespace std;
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

#include "Socket.h"

Socket::Socket()
  : m_sock ( -1 ), buf(malloc(MAXRECV)), bufLen(MAXRECV)
{
  assert(buf);
  memset ( &m_addr, 0, sizeof ( m_addr ) );
}

Socket::~Socket()
{
  if ( isValid() ) ::close ( m_sock );
  free(buf);
}

Boolean 
Socket::create()
{
  m_sock = socket ( AF_INET, SOCK_STREAM, 0 );

  if ( ! isValid() ) return false;

  // TIME_WAIT - argh
  int on = 1;
  int status = setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, 
			    ( const char* ) &on, sizeof ( on ) );

  return ( status != -1);
}

Boolean 
Socket::bind ( const int port )
{
  if ( ! isValid() ) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );

  int bind_return = ::bind ( m_sock,
			     ( struct sockaddr * ) &m_addr,
			     sizeof ( m_addr ) );

  return ( bind_return != -1 );
}

Boolean 
Socket::listen() const
{
  if ( ! isValid() ) return false;

  int listen_return = ::listen ( m_sock, MAXCONNECTIONS );

  return ( listen_return != -1 );
}

Boolean 
Socket::accept ( Socket& new_socket ) const
{
  int addr_length = sizeof ( m_addr );
  new_socket.m_sock = ::accept ( m_sock, 
				 ( sockaddr * ) &m_addr, 
				 ( socklen_t * ) &addr_length );

  return ( new_socket.m_sock > 0);
}


Boolean 
Socket::send ( const char *s ) const
{
  int status = ::send ( m_sock, s, strlen(s), 
#ifdef __APPLE__
			MSG_HAVEMORE
#else			
			MSG_NOSIGNAL
#endif
			);

  return ( status != -1);
}

char *
Socket::recv ()
{
  unsigned bufOffset = 0;
  int status;

  while (1) {
    status = ::recv ( m_sock, (char *)buf + bufOffset, bufLen - bufOffset, 0 );

    if (status == -1 || status == 0) return 0;

    if (status >= bufLen - bufOffset) {
      bufLen *= 2;
      buf = realloc(buf, bufLen); assert(buf);
      bufOffset += status;
    } else {
      if (((char *)buf)[bufOffset + status - 1] != '\0')
	((char *)buf)[bufOffset + status] = '\0';

      return (char *)buf;
    }
  }
}

Boolean
Socket::send(const void *msg, unsigned len) const
{
  int status = ::send ( m_sock, msg, len, 
#ifdef __APPLE__
			MSG_HAVEMORE
#else
			MSG_NOSIGNAL
#endif
			);

  return ( status != -1);
}

void *
Socket::recv(unsigned &len)
{
  unsigned bufOffset = 0;
  int status;
  
  while (1) {
    status = ::recv ( m_sock, (char *)buf + bufOffset, bufLen - bufOffset, 0 );
    
    if (status == -1 || status == 0) return 0;
    
    if (status >= bufLen - bufOffset) {
      bufLen *= 2;
      buf = realloc(buf, bufLen); assert(buf);
      bufOffset += status;
    } else {
      len = bufOffset + status;
      return buf;
    }
  }
}

Boolean 
Socket::connect ( const char *host, const int port )
{
  if ( ! isValid() ) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons ( port );

  int status = inet_pton ( AF_INET, host, &m_addr.sin_addr );

  if ( errno == EAFNOSUPPORT ) return false;

  status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

  return ( status == 0 );
}

void 
Socket::setNonBlocking ( const Boolean b )
{
  int opts;

  opts = fcntl ( m_sock, F_GETFL );

  if ( opts < 0 ) return;

  if ( b )
    opts = ( opts | O_NONBLOCK );
  else
    opts = ( opts & ~O_NONBLOCK );

  fcntl ( m_sock, F_SETFL, opts );
}
