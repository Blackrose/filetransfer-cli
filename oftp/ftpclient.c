#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <string.h> 
#include <dirent.h> 
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>

#include "client_process.h"

void usage()
{
    printf(" Usage:client ip_address port\n");
}

int main(int argc,char **argv)
{
    struct sockaddr_in server;
    int sockfd;
    int port;
    int ip_addr;
    int ret;

    if ( 3 != argc ){
        usage();
        return -1;
    }

    sockfd = socket(PF_INET, SOCK_STREAM , 0);
    if(sockfd < 0)
    {
        perror("data stream socket create failed!\n");
        return -1;
    }

    server.sin_family = AF_INET;
    port = atoi(argv[2]);
    server.sin_port = htons(port);

    ret = inet_pton(AF_INET, argv[1], &server.sin_addr.s_addr);
    if ( ret <=0 ){
        printf("your ip address is unvalide!\n");
        usage();
        return -1;
    }


    ret = connect(sockfd,(struct sockaddr*) &server,sizeof(struct sockaddr_in));
    if(ret < 0)
    {
        perror("data connection is failed!\n");
        return -1;
    }

    printf("connect to server successfully!\n");

    clinet_process( sockfd );
}


