
#include "serv_client.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sqlite3.h>

#define BUF_MAX 10

int  handle_request( msg_head_ctrl_t * msg, int sockfd, SSL *ssl);
void authentication(int sockfd, SSL *ssl);

void serv_client(int sockfd, SSL *ssl)
//void serv_client(int sockfd)
{
	msg_head_ctrl_t *msg;
   	ssize_t size;;
   	uint32_t total_len;
   	int ret;
   	fd_set  fs_client;
   	struct timeval timeout;

	/* verify user info */
	//authentication(sockfd);
	authentication( sockfd, ssl);

   	while(1){
    	FD_ZERO(&fs_client);
        FD_SET( sockfd, &fs_client);
		//FD_SET(ssl, &fs_client);
        timeout.tv_sec = DEFAULT_TIME_OUT;
        timeout.tv_usec = 0;

		//ret = select(ssl + 1, &fs_client, NULL, NULL, &timeout);
        ret = select(sockfd+1, &fs_client, NULL, NULL, &timeout);
       	if ( 0 == ret ){
       	//time out
           	printf("time out....\n");
           	return ;
       	}
       	if ( -1 == ret ){
           	continue;
       	}
	
		//if(FD_ISSET(ssl, &fs_client)){
		if(FD_ISSET(sockfd, &fs_client)){

       		//total_len = ntohl(body_len) + sizeof(msg_head_ctrl_t);
	   		total_len = MAX_READ_BYTES + sizeof(msg_head_ctrl_t);
       		msg = malloc(total_len);
       		if ( NULL == msg ){
           		printf("malloc error!\n");
           		return ;
       		}

       		memset(msg, 0 , total_len);
       		size = SSL_read( ssl, msg, total_len);
       		//size = read( sockfd, msg, total_len);
		
       		msg->body_len = ntohl(msg->body_len);
       		msg->command  = ntohl(msg->command);

      		printf("body_len = %d, command=%d, msg_body = %s\n", msg->body_len, msg->command, msg->msg_body);
       		if ( COMMAND_QUIT == msg->command ){
           		printf("client closed connection!\n");
				SSL_shutdown(ssl);
				SSL_free(ssl);
        		//close(clientfd);
				SSL_CTX_free(ctx);
           		close(sockfd);
           		return;
       		}
       		ret = handle_request(msg, sockfd, ssl);
       		if ( SOCK_ERROR == ret ){
           		return;
       		}

		}
       	free(msg);
   	}

}

/*
 *	FunctionName: authentication()
 *	Description: this function just verify who isn't member ,
 *			when user login on this server.
 * 
 */

void authentication(int sockfd, SSL *ssl)
{

	
	sqlite3 *db;
	char sql[100];
	int res = 0;
	char *errmsg=NULL;
	int nrow=0,ncolumn=0;
	char **Result;

	int flag = 0;
	struct info{
		char username[BUF_MAX];
		char passwd[BUF_MAX];
	};
	struct info test;

	while(!flag){
		memset(test.username, 0, BUF_MAX);
		memset(test.passwd, 0, BUF_MAX);

		SSL_read(ssl, &test, sizeof(struct info));
		//read(sockfd, &test, sizeof(struct info));
		sprintf(&sql, "select * from userinfo where username='%s' and password='%s';", test.username, test.passwd);
		printf("un:%s %d\tpw:%s %d\n", test.username, strlen(test.username), test.passwd, strlen(test.passwd));

			
		if(sqlite3_open("info.db3", &db) != 0){
			printf("database open error!\n");
			exit(1);
		}
		res = sqlite3_exec(db, sql, NULL, 0, &errmsg);

		sqlite3_get_table(db,sql,&Result,&nrow,&ncolumn,&errmsg);
	
		//printf("row=%d column=%d\n",nrow,ncolumn);
		if(nrow==1){
			printf("***user=%s login successful!**\n",test.username);
			SSL_write(ssl, "1", 2);
			//write(sockfd, "1", 2);
			flag = 1;
			//return 1;
		}else{
			printf("~~~user=%s login failed!~~~\n",test.username);
			SSL_write(ssl, "0", 2);
			//write(sockfd, "0", 2);
			//return 0;
		}

	}
	sqlite3_free_table(Result);
	sqlite3_close(db);

}

/*
 * FunctionName: handle_put
 * Description: handle upload request
 *
 */

int handle_put(msg_head_ctrl_t *msg, int sockfd, SSL *ssl)
{
	int ret;
	int fd = -1; 
	ssize_t read_bytes, write_bytes;
	msg_head_ctrl_t *ack_msg;
	
	fd_set fs_client;

	ack_msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);

		
	while(true){
		FD_ZERO(&fs_client);
       	FD_SET(sockfd, &fs_client);

		//if(FD_ISSET(sockfd, &fs_client)){
			memset(ack_msg, 0, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);

			ret = SSL_read(ssl, ack_msg, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
			//ret = read(sockfd, ack_msg, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
			
			ack_msg->body_len = ntohl(ack_msg->body_len);
			ack_msg->command  = ntohl(ack_msg->command);

			printf("command=%d, body_len=%d\n", ack_msg->command, ack_msg->body_len);
			if( -1 == fd){
				fd = open(msg->msg_body, O_WRONLY | O_CREAT | O_EXCL, 0660);
				if ( -1 == fd){
                	printf("create file %s failed! %s\n", msg->msg_body, strerror(errno));
                	ret = FAILED;
                	goto Exit;
            	}

			}

		    ret = write(fd, ack_msg->msg_body, ack_msg->body_len);
			if( COMMAND_END_FILE == ack_msg->command){
				ret = SUCCESSFUL;
				printf("receiver over!\n");
				goto Exit;
			}
		//}

	}


Exit:
   free(ack_msg);
   close(fd);
   return ret;
}

/*
 * FunctionName: handle_get
 * Description: handle client request download file
 *
 */

int handle_get(msg_head_ctrl_t * msg, int sockfd, SSL *ssl)
{
    int fd;
    int ret;
    bool exist;
    uint32_t body_len;
    msg_head_ctrl_t *ack_msg;
    ssize_t  read_bytes;
    ssize_t  sent_bytes;

    exist = file_exist( msg->msg_body );
    printf("body=%s ,file exist=%d\n", msg->msg_body, exist);
	
	/* when ftp server hasn't exist file of request then run next code */
    if ( false == exist ){
        printf("File %s doesn't exist!\n",msg->msg_body);
        body_len = msg->body_len;
        msg->body_len = htonl(msg->body_len);
        msg->command  = htonl(COMMAND_NO_FILE);
        ret = SSL_write(ssl, msg, sizeof(msg_head_ctrl_t) + body_len);
		//ret = send(sockfd, msg, sizeof(msg_head_ctrl_t) + body_len, 0);
  
        if ( ( sizeof(msg_head_ctrl_t) + body_len ) != ret){
           printf("send error,%s:%d",__FUNCTION__, __LINE__);
           return SOCK_ERROR;
        }

        return FAILED;
    }

   
	/* file exist */
    fd = open( msg->msg_body, O_RDONLY);
    if ( -1 == fd){
        printf("create file %s failed! %s:%d\n", msg->msg_body,__FUNCTION__, __LINE__);
        return FAILED;
    }

    ack_msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);

    while(true){

		/* read content from file */
        read_bytes = read(fd, ack_msg->msg_body, MAX_READ_BYTES);
        if ( -1 == read_bytes ){
            read_bytes = 0;
            ack_msg->body_len = 0;
            ack_msg->command  = htonl(COMMAND_ERROR_FILE);
            ret = FAILED;
        }
        else if ( 0 == read_bytes){
            ack_msg->body_len = 0;
            ack_msg->command  = htonl(COMMAND_END_FILE);
            ret = SUCCESSFUL;
        }
		else if(read_bytes < MAX_READ_BYTES){
			ack_msg->body_len = htonl(read_bytes);
			ack_msg->command  = htonl(COMMAND_END_FILE);
			ret = SUCCESSFUL;
		}
		else if ( read_bytes > 0 ){
            //printf("len %d,  %d\n", read_bytes, ack_msg->body_len);
            ack_msg->body_len = htonl(read_bytes);
            ack_msg->command  = htonl(msg->command);
            ret = SUCCESSFUL;
        }

        sent_bytes = SSL_write( ssl, ack_msg, read_bytes + sizeof(msg_head_ctrl_t));
		//sent_bytes = write(sockfd, ack_msg, read_bytes + sizeof(msg_head_ctrl_t));
        if ( sent_bytes != (read_bytes + sizeof(msg_head_ctrl_t))){
            ret = SOCK_ERROR;
            printf("send data error!%s:%d",__FUNCTION__, __LINE__);
            break;
        }
        if ( 0 == ack_msg->body_len || ack_msg->body_len < MAX_READ_BYTES){ 
            break;
        }
    }
	
	printf("send over!\n");
    close(fd);
    return ret;
}




int handle_ls_pwd(msg_head_ctrl_t * msg, int sockfd, SSL *ssl)
{
   FILE *file;
   msg_head_ctrl_t *ack_msg;
   size_t read_bytes;
   size_t ret;


   printf("command=%d,body=%s\n", msg->command, msg->msg_body);
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
       
	   ret = SSL_write(ssl, ack_msg, read_bytes + sizeof(msg_head_ctrl_t));
	   //ret = send(sockfd, ack_msg, read_bytes+sizeof(msg_head_ctrl_t), 0);
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

/*
 * FunctionName: handle_request
 * Description: handle all request by client
 *
 */

int handle_request( msg_head_ctrl_t * msg, int sockfd, SSL *ssl)
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
		case COMMAND_RM:
		case COMMAND_MKDIR:
           	ret = handle_ls_pwd(msg, sockfd, ssl);
           	break;

       	case COMMAND_GET:
           	ret = handle_get(msg, sockfd, ssl);
           	break;
       	case COMMAND_PUT:
			ret = handle_put(msg, sockfd, ssl);
           	break;
       	default:
           	ret = FAILED;
           	break;
   	}

   return ret;
}


