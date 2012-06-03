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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>


#include "common.h"

#define DEFAULT_CLIENT_TIME_OUT 2

	SSL_CTX *ctx;
	SSL *ssl;


//void clinet_process(int  sockfd );
void client_process(int sockfd, SSL *ssl);
void print_client_prompt();

#endif
