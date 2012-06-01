#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


#include "common.h"


char * trim_left_space( char * str)
{
    while( *str && isspace( *str )){
        str++;
    }

    return str;
}

char *trim_all_space( char *str)
{
    char *ret;

    ret = trim_left_space(str);
    str = ret;
    while( *str && !isspace( *str )){
        str++;
    }
    *str = '\0';

    return ret;
}


command_type_t get_command_type( char *str )
{
    command_type_t ret = INVALID_COMMAND_TYPE;

    

    if ( 0 == memcmp( str, PUT_COMMAND_STR, strlen(PUT_COMMAND_STR)) ){
        ret = COMMAND_PUT;
    }
    else if ( 0 == memcmp( str, GET_COMMAND_STR, strlen(GET_COMMAND_STR))){
        ret = COMMAND_GET;
    }
    else if ( 0 == memcmp( str, CD_COMMAND_STR, strlen(CD_COMMAND_STR))){
        ret = COMMAND_CD;
    }
    else if ( 0 == memcmp( str, LS_COMMAND_STR, strlen(LS_COMMAND_STR))){
        ret = COMMAND_LS;
    }
    else if ( 0 == memcmp( str, PWD_COMMAND_STR, strlen(PWD_COMMAND_STR))){
        ret = COMMAND_PWD;
    }
    else if ( 0 == memcmp( str, LCD_COMMAND_STR, strlen(LCD_COMMAND_STR))){
        ret = COMMAND_LCD;
    }
    else if ( 0 == memcmp( str, LLS_COMMAND_STR, strlen(LLS_COMMAND_STR))){
        ret = COMMAND_LLS; 
    }
    else if ( 0 == memcmp( str, LPWD_COMMAND_STR, strlen(LPWD_COMMAND_STR))){
        ret = COMMAND_LPWD;
    }
    else if ( 0 == memcmp( str, QUIT_COMMAND_STR, strlen(QUIT_COMMAND_STR))){
        ret = COMMAND_QUIT;
    }
    else {
        printf(PROMPT_STR"%s\n", COMMAND_ERR_STR);
    }

    return ret;
}


void save_file_to_disk( char *file_name)
{
}

bool  file_exist(char *path_name)
{
   struct stat buf;
   
   if ( -1 == lstat(path_name, &buf) ){
       return false;
   }

    if ( S_ISREG( buf.st_mode) ){
        return  true;
    }
      
   return false;
}

