#include "app_fx.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ffsys.h"

int32_t set_coresize(void)
{
    return 0;
}

int32_t daemon_init(void)
{
    return 0;
}

int32_t write_pid(char* file_name)
{
    FILE* lfile = fopen(file_name, "w");
    if (lfile == NULL) return -1;
    fprintf(lfile, "%lu", (unsigned long)GetCurrentProcessId());
    fclose(lfile);
    return 0;
}

int32_t read_pid(char* file_name)
{
    FILE* lfile = fopen(file_name, "r");
    if (lfile == NULL) return -1;
    unsigned long pid = 0;
    if (fscanf(lfile, "%lu", &pid) != 1) { fclose(lfile); return -1; }
    fclose(lfile);
    return (int32_t)pid;
}

int32_t pre_main(int32_t argc, char* argv[], char* cfg_filename)
{
    for (int32_t i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-c") == 0)
        {
            if (i + 1 >= argc) return -1;
            ff_snprintf(cfg_filename, MAX_FILENAME, "%s", argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printf("Usage:\n    -c filename: start with config file.\n");
            return -1;
        }
    }
    return 0;
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>

#include "autolock.h"
#include "log.h"

int32_t set_coresize(void)
{
    struct rlimit rlim_core;
    rlim_core.rlim_cur = RLIM_INFINITY;
    rlim_core.rlim_max = RLIM_INFINITY;
    return setrlimit(RLIMIT_CORE, &rlim_core);
}

int32_t daemon_init(void)
{
    pid_t pid;
    if ((pid = fork()) != 0) exit(0);
    setsid();
    if ((pid = fork()) != 0) exit(0);
    umask(0);
    for (int32_t i = 0; i < 3; i++) close(i);
    return 0;
}

int32_t write_pid(char* file_name)
{
    pid_t cur_pid = getpid();
    FILE* lfile = fopen(file_name, "w");
    if (lfile == NULL) return -1;
    fprintf(lfile, "%d", (int32_t)cur_pid);
    fflush(lfile);
    fclose(lfile);
    chmod(file_name, S_IRUSR | S_IWUSR);
    return 0;
}

int32_t read_pid(char* file_name)
{
    FILE* lfile = fopen(file_name, "r");
    if (lfile == NULL) return -1;
    pid_t cur_pid;
    if (fscanf(lfile, "%d", &cur_pid) != 1) { fclose(lfile); return -1; }
    fclose(lfile);
    return cur_pid;
}

int32_t pre_main(int32_t argc, char* argv[], char* cfg_filename)
{
    int32_t ch;
    char* strvar = NULL;
    opterr = 0;

    while ((ch = getopt(argc, argv, "c:")) != EOF)
    {
        switch (ch)
        {
        case 'c':
            strvar = optarg;
            ff_snprintf(cfg_filename, MAX_FILENAME, "%s", strvar);
            break;
        default:
            printf("Usage : \n    -c filename: start with config file.\n ");
            return -1;
        }
    }

    if (set_coresize() < 0)
    {
        FLOG((char *)"./error.log", (char *)"Failed to set corefile size to unlimited! [%s]\n", strerror(errno));
        return -1;
    }

    return 0;
}

#endif
