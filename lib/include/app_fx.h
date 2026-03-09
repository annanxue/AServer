#ifndef __APP_FX_H__
#define __APP_FX_H__

#include <stdint.h>

int32_t write_pid( char* _file_name );
int32_t read_pid( char* _file_name );
int32_t pre_main( int32_t argc, char* argv[], char* _cfg_file_name );
int32_t daemon_init(void);

const int32_t MAX_FILENAME = 256;

#endif
