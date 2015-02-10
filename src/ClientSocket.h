/*
 * ClientSocket.h --
 *     Wrapper class for Berkeley sockets on client side
 *
 * Yi Su <suy@jhu.edu>
 *     based on examples given by Rob Tougher <rtougher@yahoo.com>
 *     in "Linux Socket Programming In C++", Issue 74 of Linux Gazette, 
 *     January 2002 <http://linuxgazette.net/issue74/tougher.html>
 */

#ifndef _ClientSocket_h_
#define _ClientSocket_h_

#include "Socket.h"

class ClientSocket : public Socket
{
 public:
  ClientSocket ( const char *host, int port );
  virtual ~ClientSocket() {};

  const ClientSocket& operator << ( const char * );
  const ClientSocket& operator >> ( char *& );
};

#endif /* _ClientSocket_h_ */
