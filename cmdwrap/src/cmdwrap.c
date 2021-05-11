#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#define MAX_WRAP_ARGS 1024

static void *read_text_file(const char *pPath)
{
    char *buff = NULL;
    size_t size = 0; 
    FILE *f = fopen(pPath, "r");
    if (f != NULL) {
        fseek (f, 0, SEEK_END);
        size = ftell(f);
        fseek (f, 0, SEEK_SET);
        if (size > 0) {
            buff = malloc(size + 1);
            if (NULL == buff || size != fread(buff, 1, size, f)) {
                free(buff);
                buff = NULL;
            } else {
                buff[size] = '\0';
            }
            fclose(f);
        }
    }
    return buff;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        printf("Version: 2\n");
        printf("%s input_file args_splitter [priority]\n", argv[0]);
        printf("  input_file: text file with command\n");
        printf("  args_split: used arguments splitter\n");
        printf("  priority: call setpriority with priority value before exec\n");
        printf("\n");
        return 0;
    }
    
    if (argc == 4) {
        int prio = atoi(argv[3]); 
        setpriority(PRIO_PROCESS, 0, prio);
        argc -= 1;
    }
    
    if (argc == 3 && argv[1][0] != '\0' && argv[2][0] != '\0') {
        char *ptr = read_text_file(argv[1]);
        if (ptr) {
            char * argv_wrap[MAX_WRAP_ARGS];
            unsigned int i = 0;
            size_t sep_len = strlen(argv[2]);
            memset(argv_wrap, 0x00, sizeof(argv_wrap));
            for (i=0; i < (MAX_WRAP_ARGS - 1); ++i) {
                argv_wrap[i] = ptr;
                ptr = strstr(ptr, argv[2]);
                if (ptr != NULL) {
                    *ptr = '\0';
                    ptr += sep_len;
                }
                else {
                    break;
                }
            }
            
            execvp(argv_wrap[0], argv_wrap);
            return errno;
        }
        return -2;
    }
    return -1;
}
