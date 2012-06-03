#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "client_process.h"

#define BUF_MAX 10
void login(int sockfd, SSL *ssl); 
int handle_server_ack(int sockfd, SSL *ssl);
int handle_user_input( char *user_input, int sockfd, SSL *ssl);


void  client_process(int  sockfd, SSL *ssl)
{
    fd_set fs_read;
    int max_sock;
    char buf[255];
	int ret;

    max_sock = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
	
	// call login function login to server
	login(sockfd, ssl);

	printf("If you don't know do anything, please type 'help', it can help you :)!\n");
	print_client_prompt();
	
    while(1){
        FD_ZERO( &fs_read);
        FD_SET( STDIN_FILENO, &fs_read);
        FD_SET( sockfd, &fs_read);

       if (  select ( max_sock+1, &fs_read, NULL, NULL, NULL) <=0 ){
           continue;
       }

       if ( FD_ISSET( sockfd, &fs_read ) ){
           ret = handle_server_ack(sockfd, ssl);
           if ( SOCK_ERROR == ret ){
               break;
           }
       }

       if ( FD_ISSET( STDIN_FILENO, &fs_read ) ){
           memset(buf, 0 ,255);

           fgets( buf, 255, stdin);
           buf[strlen(buf)-1] = '\0';//delete '\n'
           ret = handle_user_input(buf, sockfd, ssl);
           if ( SOCK_ERROR == ret ){
               break;
           }

       }
       print_client_prompt();
    }

}



int handle_server_ack(int sockfd, SSL *ssl)
{
    int ret;
    
    uint32_t  total_len;
    msg_head_ctrl_t *msg;

#if 0
    ret = recv( sockfd, &body_len , sizeof(body_len), MSG_PEEK);
    if ( ret<=0 ){
        printf("connectionn lost ... (handle_server1)\n");
        exit(-1);
    }
#endif
    
	total_len = MAX_READ_BYTES + sizeof(msg_head_ctrl_t);
    msg = malloc( total_len);
    if ( NULL == msg ){
        perror("out of memeory\n");
        return FAILED;
    }
    memset( msg, 0, total_len);

    ret = SSL_read( ssl, msg , total_len);
	//ret = read(sockfd, msg, total_len);
    if ( ret<=0 ){
        printf("connectionn lost ... (handle_server2)\n");
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
	return SUCCESSFUL;
}

int send_command_to_server( int sockfd, SSL *ssl, char *user_input, command_type_t type)
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
	memset(msg, 0, total_len);

    memcpy(msg->msg_body, user_input, body_len);
	//printf("len = %d, %s, body = %s\n", body_len, user_input, msg->msg_body);
    msg->command = htonl(type);
    msg->body_len = htonl(body_len);

    ret = SSL_write(ssl, msg, total_len);
	//ret = write(sockfd, msg, total_len);
    if ( ret < total_len ){
        printf("connection lost ...(send command)\n");
        free(msg);
        return SOCK_ERROR;
    }       

    free(msg);
    return SUCCESSFUL;
}


int put_file_to_server(int sockfd, SSL *ssl, char *user_input, command_type_t type)
{
	int fd = -1;
	int ret;
	ssize_t read_bytes, write_bytes;
	msg_head_ctrl_t *msg;

	
	ret = send_command_to_server(sockfd, ssl, user_input, type);
	if( -1 == fd){
			fd = open(user_input, O_RDONLY);
	}
	
	while(true){
		// read content 
		msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
		read_bytes = read(fd, msg->msg_body, MAX_READ_BYTES);
		 if ( -1 == read_bytes ){
            read_bytes = 0;
            msg->body_len = 0;
            msg->command  = htonl(COMMAND_ERROR_FILE);
            ret = FAILED;
        }
        else if ( 0 == read_bytes){
            msg->body_len = 0;
            msg->command  = htonl(COMMAND_END_FILE);
            ret = SUCCESSFUL;
        }
		else if(read_bytes < MAX_READ_BYTES){
			msg->body_len = htonl(read_bytes);
			msg->command  = htonl(COMMAND_END_FILE);
			ret = SUCCESSFUL;
		}
		else if ( read_bytes > 0 ){
            msg->body_len = htonl(read_bytes);
            msg->command  = htonl(msg->command);
            ret = SUCCESSFUL;
        }

		write_bytes = SSL_write( ssl, msg, sizeof(msg_head_ctrl_t) + read_bytes);
		//write_bytes = write(sockfd, msg, sizeof(msg_head_ctrl_t) + read_bytes);
		if ( write_bytes != (read_bytes + sizeof(msg_head_ctrl_t))){
            ret = SOCK_ERROR;
            printf("send data error!%s:%d",__FUNCTION__, __LINE__);
            break;
        }
        if ( COMMAND_END_FILE == ntohl(msg->command)){ 
			goto Exit;
        }

	}
Exit:
	free(msg);
	close(fd);
	return ret;


}

int get_file_from_server(int sockfd, SSL *ssl, char *user_input, command_type_t type)
{
    int ret;
    int fd=-1;
    
    fd_set  fs_client;
    struct timeval timeout;
    msg_head_ctrl_t *msg;

    ret = send_command_to_server(sockfd, ssl, user_input, type);
    if ( ret != SUCCESSFUL){
       return ret;
    }

    msg = malloc(sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
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
      
       memset(msg, 0, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);

       ret = SSL_read( ssl, msg, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
	   //ret = read(sockfd, msg, sizeof(msg_head_ctrl_t) + MAX_READ_BYTES);
#if 0       
	   if ( (sizeof(msg_head_ctrl_t) + len) != ret){
           printf("recv msg error!%s:%d, recv %d, acut %d\n", __FUNCTION__, __LINE__,ret,len );
           ret = SOCK_ERROR;
           goto Exit;
       }
#endif
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



int handle_user_input( char *user_input, int sockfd, SSL *ssl)
{
    command_type_t type;
    int ret;
    bool exist;

    ret = SUCCESSFUL;
    user_input = trim_left_space(user_input);
    type = get_command_type(user_input);
    switch(type){
		case COMMAND_HELP:
			printf("this ftp provide these command:\ncd: change directory. cd Directory\nls: list content of current directory. ls Directory\npwd: print the current work directory.\nmkdir: create a directory on server. mkdir Directory\nrm: remove directory or file on server. rm Directory|filename\nget: download file from server. get filename\nput:upload file to server. put filename\nquit: you know what it is!^_^\n");
			break;

        case COMMAND_CD:
        case COMMAND_LS:
        case COMMAND_PWD:
		case COMMAND_MKDIR:
		case COMMAND_RM:
        case COMMAND_QUIT:            
            ret = send_command_to_server(sockfd, ssl, user_input, type);
            break;

        case COMMAND_LPWD:
        case COMMAND_PUT:
			user_input = trim_all_space(user_input + 3);// skip "put" characters
			exist = file_exist(user_input);
			if( false == exist ){
				printf("your computer haven't exist file '%s'!\n", user_input); 
			}else{
				ret = put_file_to_server(sockfd, ssl, user_input, type);
			
			}
            break;
        case COMMAND_GET:
            user_input = trim_all_space(user_input + 3);//skip "get" characters
            exist = file_exist(user_input);
            if ( false == exist ){
                ret = get_file_from_server(sockfd, ssl, user_input, type);
            }
            else{
                printf("file %s already exist on your computer!\n", user_input);
            }
		
            break;
        default:
            printf("your command doesn't support!\n");
            ret = FAILED;
            break;
    }

    if ( COMMAND_QUIT == type ){
		printf("See you then!\n");
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		close(sockfd);
        exit(0);
    }

    return ret;
}


/*
 *	FunctionName: login()
 *	Description: this function finish a login action.
 *			Help user login to ftp-server, and tell user
 *			isn't succeeful.
 *
 */

void login(int sockfd, SSL *ssl)
{
	// verify username and password
	int flag = 0;
	struct info{
		char username[BUF_MAX];
		char passwd[BUF_MAX];
	};
	struct info test;

	while(!flag){
		
		printf("Please login before operation!\n");
		printf("Username:");
		
		fgets(test.username, BUF_MAX, stdin);
		test.username[strlen(test.username) -1] ='\0';
		
		printf("Password:");
		fgets(test.passwd, BUF_MAX, stdin);
		test.passwd[strlen(test.passwd) -1] ='\0';
		
		fflush(stdout);

		SSL_write(ssl, &test, sizeof(struct info));
		//write(sockfd, &test, sizeof(struct info));
	
		char buff[2];
		
		SSL_read(ssl, buff, 2);
		//read(sockfd, buff, 2);
		printf("buff=%s\n", buff);
		flag = atoi(buff);
		printf("flag=%d\n", flag);
	}


}

void  print_client_prompt(){
   printf(PROMPT_STR);
   fflush(stdout);
}

