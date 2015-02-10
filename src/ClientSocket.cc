/*
 * ClientSocket.cc --
 *     Wrapper class for Berkeley sockets on client side
 *
 * Yi Su <suy@jhu.edu>
 *     based on examples given by Rob Tougher <rtougher@yahoo.com>
 *     in "Linux Socket Programming In C++", Issue 74 of Linux Gazette, 
 *     January 2002 <http://linuxgazette.net/issue74/tougher.html>
 */

#include <stdlib.h>

#include "ClientSocket.h"

ClientSocket::ClientSocket ( const char *host, int port )
{
  if ( ! Socket::create() ) {
    dout() << "Could not create client socket." << endl;
    exit(1);
  }

  if ( ! Socket::connect ( host, port ) ) {
    dout() << "Could not bind to port." << endl;
    exit(1);
  }
}


const ClientSocket& ClientSocket::operator << ( const char *s )
{
  if ( ! Socket::send ( s ) )
    dout() << "Could not write to socket." << endl;

  return *this;
}


const ClientSocket& ClientSocket::operator >> ( char *&s )
{
  s = Socket::recv();
  
  if (s == 0) 
    dout() << "Could not read from socket." << endl;

  return *this;
}
