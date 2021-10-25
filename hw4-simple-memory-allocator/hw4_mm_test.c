#include "lib/hw_malloc.h"
#include "hw4_mm_test.h"

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define HEAP_INITIAL 64 * 1024
#define TOKEN_BUFSIZE 500

struct LineArgument *lineArg;

char *shell_read_line()
{
    char* line = NULL;
    size_t size = 0;
    int read;
    if((read = getline(&line, &size, stdin)) == -1) {
        printf("reached EOF\n");
        return NULL;
    }
    return line;
}

struct LineArgument* shell_split_line(char * line)
{
    int buffsize = TOKEN_BUFSIZE;
    int argc = 0;
    char **tokens = sbrk(buffsize * sizeof(char *));
    printf("malloc size for tokens %ld", buffsize * sizeof(char *));
    char *token;

    token = strtok(line, " \t\r\n\a");
    while(token != NULL) {
        tokens[argc] = token;
        argc++;
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[argc] = NULL;
    lineArg->args = tokens;
    lineArg->argc = argc;
    return lineArg;
}

void execution(struct LineArgument * lineArg)
{
    char **args = lineArg->args;
    char argc = lineArg->argc;
    // Do things here
    if(!strcmp(args[0], "alloc") && argc == 2) {
        // alloc N
        int bytes = atoi(args[1]);
        chunk_header_t * data = hw_malloc(bytes);
        chunk_header_t * head = (void*)data - 24;
        if(head->chuck_info.mmap_flag) {
            printf("%p\n", data);
        } else {
            printf("%p\n", (void *)((void *)data - start_brk));
        }
    } else if (!strcmp(args[0], "free") && argc == 2) {
        // free ADDR
        void * ptr;
        char * tmp;
        size_t address = strtol(args[1] + 2, &tmp, 16);
        ptr = (void *)address;
        int success = hw_free(ptr);
        if(success != 0) {
            printf("success\n");
        } else {
            printf("fail\n");
        }
    } else if (!strcmp(args[0], "print")) {
        // print bin[i] or mmap_alloc_list
        printf("\n");
        if(!strcmp(args[1], "bin[10]")) {
            printList(bin[10]);
        } else if(strlen(args[1]) == 6) {
            printList(bin[args[1][4] - 48]);
        } else if(!strcmp(args[1], "mmap_alloc_list")) {
            // mmap_alloc_list
            printList(mmap_alloc_list);
        } else if(!strcmp(args[1], "bins")) {
            // mmap_alloc_list
            printBins();
        } else if(!strcmp(args[1], "heap")) {
            // mmap_alloc_list
            printList(allocated_heap);
        } else if(!strcmp(args[1], "mmap")) {
            // mmap_alloc_list
            printList(mmap_alloc_list);
        }
    }
    return;
}

void shell_loop()
{
    // read a line
    char *line;

    do {
        // printf("$ ");
        line = shell_read_line();
        if(line == NULL) {
            return;
        }
        struct LineArgument *lineArg = shell_split_line(line);
        execution(lineArg);
    } while(1);
    return;
}

int main(int argc, char *argv[])
{
    lineArg = (struct LineArgument*)sbrk(sizeof(struct LineArgument));
    shell_loop();
    return 0;
}
