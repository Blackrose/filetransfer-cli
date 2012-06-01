#include "stdlib.h"
#include "serv_client.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



int  handle_request( msg_head_ctrl_t * msg, int sockfd);

void serv_client(int sockfd)
{
   msg_head_ctrl_t *msg;
   ssize_t size;;
   uint32_t body_len;
   uint32_t total_len;
   int ret;
   fd_set  fs_client;
   struct timeval timeout;

   while(1){
        FD_ZERO(&fs_client);
        FD_SET( sockfd, &fs_client);
        timeout.tv_sec = DEFAULT_TIME_OUT;
        timeout.tv_usec = 0;

        ret = select(sockfd+1, &fs_client, NULL, NULL, &timeout);
       if ( 0 == ret ){
       //time out
           printf("time out....\n");
           return ;
       }
       if ( -1 == ret ){
           continue;
       }

       size = recv( sockfd, &body_len, sizeof(uint32_t), MSG_PEEK); 
		
       printf("recv MSG_PEEK : %d\tbody_len : %d\n", size, ntohl(body_len));

       if ( size< 0 ){
           printf("recv msg error!\n");
           return ;
       }
       if ( size == 0 ){
          printf("client close the connection\n");
          return ;
       }
 
       total_len = ntohl(body_len) + sizeof(msg_head_ctrl_t);
       msg = malloc(total_len + 1);
       if ( NULL ==msg ){
           printf("malloc error!\n");
           return ;
       }
       memset(msg, 0 , total_len+1);

       size = recv( sockfd, msg, total_len, 0);
       if ( size != total_len){
           printf("recv msg body failed!\n");
           return ;
       }

       msg->body_len = ntohl(msg->body_len);
       msg->command  = ntohl(msg->command);

//       printf("len = %d, msg = %s\n", msg->body_len, msg->msg_body);
       if ( COMMAND_QUIT == msg->command ){
           printf("client closed connection!\n");
           close(sockfd);
           return ;
       }
       ret = handle_request(msg, sockfd);
       if ( SOCK_ERROR == ret ){
           return;
       }

       free(msg);
   }

}


int handle_get(msg_head_ctrl_t * msg, int sockfd)
{
    int fd;
    int ret;
    bool exist;
    uint32_t body_len;
    msg_head_ctrl_t *ack_msg;
    ssize_t  read_bytes;
    ssize_t  sent_bytes;

    exist = file_exist( msg->msg_body );
    printf("%s , %d", msg->msg_body, exist);
    if ( false == exist ){
        printf("File %s doesn't exist!\n",msg->msg_body);
        body_len = msg->body_len;
        msg->body_len = htonl(msg->body_len);
        msg->command  = htonl(COMMAND_NO_FILE);
        ret = send(sockfd, msg, sizeof(msg_head_ctrl_t) + body_len, 0);
  
        if ( ( sizeof(msg_head_ctrl_t) + body_len ) != ret){
           printf("send error,%s:%d",__FUNCTION__, __LINE__);
           return SOCK_ERROR;
        }

        return FAILED;
    }

    


    fd = open( msg->msg_body, O_RDONLY);
    if ( -1 == fd){
        printf("create file %s failed! %s:%d\n", msg->msg_body,__FUNCTION__, __LINE__);
        return FAILED;
    }

    ack_msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);

    while(true){
        read_bytes = read(fd, ack_msg->msg_body, MAX_READ_BYTES);
        if ( read_bytes > 0 ){
            //printf("len %d,  %d\n", read_bytes, ack_msg->body_len);
            ack_msg->body_len = htonl(read_bytes);
            ack_msg->command  = htonl(msg->command);
            ret = SUCCESSFUL;
        }
        else if ( -1 == read_bytes ){
            read_bytes = 0;
            ack_msg->body_len = 0;
            ack_msg->command  = htonl(COMMAND_ERROR_FILE);
            ret = FAILED;
        }
        else if ( 0 == read_bytes ){
            ack_msg->body_len = 0;
            ack_msg->command  = htonl(COMMAND_END_FILE);
            ret = SUCCESSFUL;
        }

        sent_bytes = send( sockfd, ack_msg, read_bytes + sizeof(msg_head_ctrl_t), 0);
        if ( sent_bytes != (read_bytes + sizeof(msg_head_ctrl_t))){
            ret = SOCK_ERROR;
            printf("send data error!%s:%d",__FUNCTION__, __LINE__);
            break;
        }
        if ( 0 == ack_msg->body_len ){ 
            break;
        }
    }

    close(fd);
    return ret;
}


int handle_ls_pwd(msg_head_ctrl_t * msg, int sockfd)
{
   FILE *file;
   msg_head_ctrl_t *ack_msg;
   size_t read_bytes;
   size_t ret;


   printf("%s\n", msg->msg_body);
   file = popen( msg->msg_body, "r");
   if ( NULL == file ){
       printf("execute command failed! %s\n", msg->msg_body);
       return FAILED;
   }

   ack_msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
   if ( NULL == ack_msg ){
       printf("out of memory!\n");
       return FAILED;
   }

   ack_msg->command = htonl( msg->command );
   while(true){
       read_bytes = fread(ack_msg->msg_body,1, MAX_READ_BYTES, file);
       if ( ferror(file)){
           break;
       }
       ack_msg->body_len = htonl(read_bytes);
       ret = send(sockfd, ack_msg, read_bytes+sizeof(msg_head_ctrl_t), 0);
       if ( (read_bytes+sizeof(msg_head_ctrl_t)) != ret ){
           printf("sock error!\n");
           return SOCK_ERROR;
       }
       if ( feof(file ) ){ 
           printf("send over!\n");          
           break;
       }
       
   }

   pclose(file);

}

int handle_request( msg_head_ctrl_t * msg, int sockfd)
{
    char *cmd;
    int ret;
    

    switch(msg->command){
       case COMMAND_CD:
          cmd = trim_all_space( msg->msg_body+2);//skip "cd" charatcer
          ret = chdir(cmd);
          if ( -1 == ret ){
              printf("%s, |%s|\n", strerror(errno), cmd);
          }

            break;
       case COMMAND_LS:
       case COMMAND_PWD:
           ret = handle_ls_pwd(msg, sockfd);
           break;

       case COMMAND_GET:
           ret = handle_get(msg, sockfd);
           break;
       case COMMAND_PUT:
           break;
       default:
           ret = FAILED;
           break;
   }

   return ret;
}


