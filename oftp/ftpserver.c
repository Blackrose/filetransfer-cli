#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <string.h> 
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "serv_client.h"


int main(int argc,char** argv)
{
    int sockfd;
    int clientfd;
    uint16_t port;
    int ret;
    pid_t pid;

    struct sockaddr_in server_addr;
    
    if ( 2 != argc ){
        printf("usage: command listen_port\n");
        return -1;
    }

    port = atoi(argv[1]);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    sockfd = socket(PF_INET,SOCK_STREAM,0);
    if (sockfd < 0)   
    {   
        perror("open data stream socket failed!\n");   
        return -1;  
    }   

    ret = bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    if(ret < 0)
    {
        perror("bind data socket failed!\n");
        return -1;
    }	

    ret = listen(sockfd, SOMAXCONN );
    if( ret < 0)
    {
        perror("listening data stream failed!\n");
        return -1;
    }

    printf("Waiting for client conneciton...\n");
    while(1)
    {
        clientfd = accept(sockfd, NULL, NULL);
        if( clientfd < 0)
        {
            perror("accept data connection error!\n");
            return -1;
        }
        pid = fork();
        if ( pid < 0 ){
            perror("fork error!\n");
        }

        if ( 0 == pid ){
            //chid process
            printf("one client come ...\n");
            serv_client(clientfd);
            return 0;
        }
        
        close(clientfd);
    }
}



