/* 
 * testSocket --
 *      Test socket class
 */

#ifdef PRE_ISO_CXX
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif

#include <stdlib.h>
#include <stdio.h>

#include "Vocab.h"
#include "ServerSocket.h"
#include "ClientSocket.h"

#define TEST_PORT 30000
#define MAX_BUF_SIZE 1000

void
runServer()
{
  ServerSocket server( TEST_PORT );
  char *buf;
  
  cerr << "running...";
  while (1) {
    ServerSocket s;
    server.accept(s);

    while (1) {
      if ((buf = s.recv()) == 0) break;
      if (!s.send(buf)) break;
    }
  }
}

void
runClient()
{
  ClientSocket client("localhost", TEST_PORT);
  char *buf;
  char tmp[100];
 
  for (int i=0; i<500; i++) {
  sprintf(tmp, "20 52 47 %u", Vocab_None);
  client << tmp;
  client >> buf;
  
  cerr << "We received this response from the server:\n\"" << buf << "\"\n";
  }
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    cerr << "usage: testSocket [server|client]\n";
    exit(1);
  }

  if (strcmp(argv[1], "server") == 0) 
    runServer();
  else
    runClient();

  return 1;
}
