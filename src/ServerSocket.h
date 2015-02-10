/*
 * ServerSocket.h --
 *     Wrapper class for Berkeley sockets on server side
 *
 * Yi Su <suy@jhu.edu>
 *     based on examples given by Rob Tougher <rtougher@yahoo.com>
 *     in "Linux Socket Programming In C++", Issue 74 of Linux Gazette, 
 *     January 2002 <http://linuxgazette.net/issue74/tougher.html>
 */

#ifndef _ServerSocket_h_
#define _ServerSocket_h_

#include "Socket.h"

class ServerSocket : public Socket
{
 public:
  ServerSocket ( int port );
  ServerSocket () {};
  virtual ~ServerSocket();

  const ServerSocket& operator << ( const char * );
  const ServerSocket& operator >> ( char *& );

  void accept ( ServerSocket& );
};

#endif /* _ServerSocket_h_ */
