#include "path_misc.h"
#include "define.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
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

        DIR* dir_handle = opendir(tmp_dir_path);
        if (dir_handle != NULL) 
        {
            closedir(dir_handle);
            continue;
        }
        //else dir_handle == NULL
        if (errno != ENOENT) 
        {
            return -1;
        }

        mkdir(tmp_dir_path, 0755); 
    }
    return 0;
}

