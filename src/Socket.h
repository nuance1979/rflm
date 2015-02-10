/*
 * Socket.h --
 *     Wrapper class for Berkeley sockets
 *
 * Yi Su <suy@jhu.edu>
 *     based on examples given by Rob Tougher <rtougher@yahoo.com>
 *     in "Linux Socket Programming In C++", Issue 74 of Linux Gazette, 
 *     January 2002 <http://linuxgazette.net/issue74/tougher.html>
 */

#ifndef _Socket_h_
#define _Socket_h_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "Boolean.h"
#include "Debug.h"

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 5000;

class Socket : public Debug
{
 public:
  Socket();
  virtual ~Socket();

  // Server initialization
  Boolean create();
  Boolean bind( const int port );
  Boolean listen() const;
  Boolean accept( Socket& ) const;

  // Client initialization
  Boolean connect( const char *host, const int port );

  // Data Transimission
  Boolean send( const char * ) const; // send and recv C strings 
  char *recv();                       // (with trailer '\0')

  Boolean send(const void *msg, unsigned len) const; // send and recv
  void *recv(unsigned &len);                         // anything (with len)

  void setNonBlocking( const Boolean );
  Boolean isValid() const { return m_sock != -1; }

 private:
  int m_sock;
  sockaddr_in m_addr;

  void *buf;
  unsigned bufLen;
};

#endif /* _Socket_h_ */
