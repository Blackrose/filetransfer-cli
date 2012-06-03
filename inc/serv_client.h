#ifndef __SERV_CLIENT_H__
#define __SERV_CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX *ctx;
SSL *ssl;

//void serv_client(int sockfd);
void serv_client(int sockfd, SSL *ssl);

#endif



