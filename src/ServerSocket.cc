/*
 * ServerSocket.cc --
 *     Wrapper class for Berkeley sockets on server side
 *
 * Yi Su <suy@jhu.edu>
 *     based on examples given by Rob Tougher <rtougher@yahoo.com>
 *     in "Linux Socket Programming In C++", Issue 74 of Linux Gazette, 
 *     January 2002 <http://linuxgazette.net/issue74/tougher.html>
 */

#include "ServerSocket.h"

ServerSocket::ServerSocket ( int port )
{
  if ( ! Socket::create() )
    dout() << "Could not create server socket." << endl;

  if ( ! Socket::bind ( port ) )
    dout() << "Could not bind to port." << endl;
  
  if ( ! Socket::listen() )
    dout() << "Could not listen to socket." << endl;
}

ServerSocket::~ServerSocket()
{
}

const ServerSocket& 
ServerSocket::operator << ( const char *s )
{
  if ( ! Socket::send ( s ) )
    dout() << "Could not write to socket." << endl;

  return *this;
}

const ServerSocket& 
ServerSocket::operator >> ( char *&s )
{
  s = Socket::recv();

  if (s == 0)
    dout() << "Could not read from socket." << endl;

  return *this;
}

void 
ServerSocket::accept ( ServerSocket& sock )
{
  if ( ! Socket::accept ( sock ) )
    dout() << "Could not accept socket." << endl;
}
