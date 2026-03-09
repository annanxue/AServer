#ifndef __PATH_MISC_H__
#define __PATH_MISC_H__

/**
 * create dir recursively
 * return 
 *      0 if success 
 *      -1 if error
 * e.g. create_dir("/a/b/c") will create dir /a/b/
 */
int create_dir(const char* const _dir);

#endif
