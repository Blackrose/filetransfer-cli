#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define PUT_COMMAND_STR "put"
#define GET_COMMAND_STR  "get"
#define CD_COMMAND_STR   "cd"
#define LS_COMMAND_STR   "ls"
#define PWD_COMMAND_STR  "pwd"

#define LCD_COMMAND_STR  "lcd"
#define LLS_COMMAND_STR  "lls"
#define LPWD_COMMAND_STR "lpwd"

#define QUIT_COMMAND_STR "quit"

#define DEFAULT_TIME_OUT  600
#define MAX_READ_BYTES  4096



#define  PROMPT_STR ">"
#define  COMMAND_ERR_STR "command error"

#define FAILED   -1
#define SUCCESSFUL 0
#define SOCK_ERROR 1

typedef enum {
    COMMAND_PUT,
    COMMAND_GET,
    COMMAND_CD,
    COMMAND_LS,
    COMMAND_PWD,

    COMMAND_LCD,
    COMMAND_LLS,
    COMMAND_LPWD,

    COMMAND_QUIT,

    COMMAND_DUP_FILE,
    COMMAND_END_FILE,
    COMMAND_NO_FILE,
    COMMAND_ERROR_FILE,

    INVALID_COMMAND_TYPE

}command_type_t;


typedef struct msg_head_ctrl{
   uint32_t        body_len;
   uint32_t        command;
   char            msg_body[0];
}msg_head_ctrl_t;

typedef struct msg_data{
    uint32_t  body_len;
    char      msg_body[0];
}msg_data_t;

char * trim_left_space( char * str );
char * trim_all_space ( char *str);
command_type_t get_command_type( char *str );
void save_file_to_disk( char *file_name);
bool  file_exist(char *path_name);

#endif
