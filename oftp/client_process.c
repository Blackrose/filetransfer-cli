#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdint.h>

#include "client_process.h"


int handle_server_ack(sockfd)
{
    int ret;
    uint32_t  body_len;
    uint32_t  total_len;
    msg_head_ctrl_t *msg;

    ret = recv( sockfd, &body_len , sizeof(body_len), MSG_PEEK);
    if ( ret<=0 ){
        printf("connectionn lost ... \n");
        exit(-1);
    }

    body_len = ntohl(body_len);
    total_len = body_len + sizeof(msg_head_ctrl_t);

    msg = malloc( total_len + 1 );
    if ( NULL == msg ){
        perror("out of memeory\n");
        return ;
    }
    memset( msg, 0, total_len + 1);

    ret = recv( sockfd, msg , total_len, MSG_WAITALL);
    if ( ret<=0 ){
        printf("connectionn lost ... \n");
        exit(-1);
    }

    msg->body_len = ntohl(msg->body_len);
    msg->command  = ntohl(msg->command);
    if ( (COMMAND_LS==msg->command) || (COMMAND_PWD==msg->command) ){
       printf("\n%s\n",msg->msg_body);
    }
    else if ( COMMAND_GET == msg->command ){
        save_file_to_disk(msg->msg_body);
    }

    free(msg);
}

int send_command_to_server( int sockfd, char *user_input, command_type_t type)
{
    ssize_t ret;
    msg_head_ctrl_t *msg;
    int body_len;
    int total_len;

    body_len = strlen(user_input);
    total_len = body_len + sizeof(msg_head_ctrl_t);
    msg = malloc(total_len);
    if ( NULL == msg ){
        printf("out of memeory!\n");
        return FAILED;
    }

    msg->command = htonl(type);
    msg->body_len = htonl(body_len);

    memcpy(msg->msg_body, user_input, body_len);
    ret = send(sockfd, msg, total_len, 0);
    if ( ret < total_len ){
        printf("connection lost ...\n");
        free(msg);
        return SOCK_ERROR;
    }       

    free(msg);
    return SUCCESSFUL;
}


int get_file_from_server(int sockfd, char *user_input, command_type_t type)
{
    int ret;
    int fd=-1;
    uint32_t len;
    fd_set  fs_client;
    struct timeval timeout;

    msg_head_ctrl_t *msg;


    ret = send_command_to_server(sockfd, user_input, type);
    if ( ret != SUCCESSFUL){
       return ret;
    }


    msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES + 1);
    if ( NULL == msg ){
        printf("out of memory!\n");
        return FAILED;
    }


    while(true){
       FD_ZERO(&fs_client);
       FD_SET( sockfd, &fs_client);

       timeout.tv_sec = DEFAULT_CLIENT_TIME_OUT;
       timeout.tv_usec = 0;

       ret = select(sockfd+1, &fs_client, NULL, NULL, NULL);
       if ( 0 == ret ){
       //time out
           printf("time out....\n");
           ret = SOCK_ERROR;
           break;
       }
       if ( -1 == ret ){
           continue;
       }
      
       memset(msg, 0, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES + 1);
       ret = recv(sockfd, &len, sizeof(msg->body_len), MSG_PEEK);
       if (ret <= 0){
           printf("sock error!\n");
           ret = SOCK_ERROR;
           goto Exit;
       }
       
       len = ntohl(len);

       ret = recv(sockfd, msg, sizeof(msg_head_ctrl_t) + len, MSG_WAITALL);
       if ( (sizeof(msg_head_ctrl_t) + len) != ret){
           printf("recv msg error!%s:%d, recv %d, acut %d\n", __FUNCTION__, __LINE__,ret,len );
           ret = SOCK_ERROR;
           goto Exit;
       }
       msg->body_len = ntohl(msg->body_len);
       msg->command  = ntohl(msg->command);

       if ( COMMAND_NO_FILE == msg->command ){
           printf("File %s doesn't exist on server\n", user_input);
           ret = FAILED;
           goto Exit;
       }
       if ( -1 == fd){
            fd = open(user_input, O_WRONLY|O_CREAT|O_EXCL, 0660);
            if ( -1 == fd){
                printf("create file %s failed! %s\n", user_input, strerror(errno));
                ret = FAILED;
                goto Exit;
            }
        }
       //save to file
       ret = write(fd, msg->msg_body, msg->body_len);
       if ( msg->body_len != ret ){
          printf("Write file failed!\n");
          ret = FAILED;
          goto Exit;
       }

       if ( COMMAND_END_FILE == msg->command ){          
           ret = SUCCESSFUL;
           goto Exit;
       }

    }

Exit:
   free(msg);
   if (SUCCESSFUL != ret ){
       unlink(user_input);
   }
   close(fd);

   return ret;

}



int handle_user_input( char *user_input, int sockfd)
{
    command_type_t type;
    int ret;
    bool exist;

    ret = SUCCESSFUL;
    user_input = trim_left_space(user_input);
    type = get_command_type(user_input);
    switch(type){
        case COMMAND_CD:
        case COMMAND_LS:
        case COMMAND_PWD:
        case COMMAND_QUIT:            
            ret = send_command_to_server(sockfd, user_input, type);
            break;
  
        case COMMAND_LCD:
            user_input = trim_all_space(user_input + 3);
            ret = chdir(user_input);
            if ( -1 == ret ){
                printf("%s, |%s|\n", strerror(errno), user_input);
            }
            break;
        case COMMAND_LLS:
        case COMMAND_LPWD:
            //skil  'l' character
            system(user_input+1);
            break;
 
        case COMMAND_PUT:
            break;
        case COMMAND_GET:
            user_input = trim_all_space(user_input+3);//skip "get" characters
            exist = file_exist(user_input);
            if ( false ==exist ){
                ret = get_file_from_server(sockfd, user_input, type);
            }
            else{
                printf("file %s already exist on your computer!\n", user_input);
            }
            break;
        default:
            printf("you command dosn't support!\n");
            ret = FAILED;
            break;
    }

    if ( COMMAND_QUIT == type ){
        exit(0);
    }

    return ret;
}

void  clinet_process(int  sockfd )
{
    fd_set fs_read;
    int max_sock;
    char buf[255];
    int ret;

    max_sock = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
    print_client_prompt();

    while(1){
        FD_ZERO( &fs_read);
        FD_SET( STDIN_FILENO, &fs_read);
        FD_SET( sockfd, &fs_read);

       if (  select ( max_sock+1, &fs_read, NULL, NULL, NULL) <=0 ){
           continue;
       }

       if ( FD_ISSET( sockfd, &fs_read ) ){
           ret = handle_server_ack(sockfd);
           if ( SOCK_ERROR == ret ){
               break;
           }
       }

       if ( FD_ISSET( STDIN_FILENO, &fs_read ) ){
           memset(buf, 0 ,255);

           fgets( buf, 255, stdin);
           buf[strlen(buf)-1] = '\0';//delete '\n'
           ret = handle_user_input(buf, sockfd);
           if ( SOCK_ERROR == ret ){
               break;
           }

       }
       
       print_client_prompt();
    }

}

void  print_client_prompt(){
   printf(PROMPT_STR);
   fflush(stdout);
}

