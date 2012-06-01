#ifndef __CLIENT_PROCESS__
#define __CLIENT_PROCESS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "common.h"

#define DEFAULT_CLIENT_TIME_OUT 2

void  clinet_process(int  sockfd );
void  print_client_prompt();
#endif
