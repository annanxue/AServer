#include "path_misc.h"
#include "define.h"

#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include <errno.h>


//-----------------------------------------------------------------------------
int create_dir(const char* const _dir)
{
    if (_dir == NULL || strlen(_dir) > MAX_PATH)
        return -1;

	char tmp_dir_path[MAX_PATH] = {0};
    const char* slash_pos = _dir;
    while ((slash_pos = strchr(slash_pos, '/')) != 0) 
    {
        int tmp_dir_len = slash_pos - _dir;
        memcpy(tmp_dir_path, _dir, tmp_dir_len);
        tmp_dir_path[ tmp_dir_len ] = '\0';
        slash_pos++;

#ifdef _WIN32
        if (_access(tmp_dir_path, 0) != 0)
        {
            if (_mkdir(tmp_dir_path) != 0) return -1;
        }
#else
        DIR* dir_handle = opendir(tmp_dir_path);
        if (dir_handle != NULL)
        {
            closedir(dir_handle);
            continue;
        }
        if (errno != ENOENT)
        {
            return -1;
        }
        mkdir(tmp_dir_path, 0755);
#endif
    }
    return 0;
}

