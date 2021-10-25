#ifndef SIMPLE_PSTREE
#define SIMPLE_PSTREE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int testCompleteNumber(char * str)
{
    int i;
    for(i = 0; i < strlen(str); i++) {
        if(str[i] > 57 || str[i] < 48) {
            return -1;
        }
    }
    return 0;
}

void substring(char s[], char sub[], int p, int l)
{
    int c = 0;

    while (c < l) {
        sub[c] = s[p+c-1];
        c++;
    }
    sub[c] = '\0';
}

int analyzedCommand(char * command, int argc,char* argv[])
{
    int pidBuff;
    char * argOpt;
    argOpt = (char*)malloc(5 * sizeof(char));
    char * argBuff;
    argBuff = (char*)malloc(10 * sizeof(char));
    if(argc == 1) {
        sprintf(command, "c 1");
        //free malloc string
        free(argBuff);
        free(argOpt);
        return 0;
    } else if(argc == 2) {
        int argSize = strlen(argv[argc - 1]);
        if(testCompleteNumber(argv[argc - 1]) == -1) {
            if(argSize < 2) {
                //free malloc string
                free(argBuff);
                free(argOpt);
                return -1;
            } else if(argSize == 2) {
                if(strcmp(argv[argc - 1], "-p") == 0) {
                    sprintf(command, "p %d",getpid());
                } else if(strcmp(argv[argc - 1], "-c") == 0) {
                    sprintf(command, "c 1");
                } else if(strcmp(argv[argc - 1], "-s") == 0) {
                    sprintf(command, "s %d",getpid());
                } else {
                    //free malloc string
                    free(argBuff);
                    free(argOpt);
                    return -1;
                }
            } else if(argSize > 2) {
                memcpy(argOpt, &argv[argc - 1][0], 2);
                memcpy(argBuff, &argv[argc - 1][2], argSize - 2);
                if(testCompleteNumber(argBuff) == -1) {
                    pidBuff = -1;
                    //free malloc string
                    free(argBuff);
                    free(argOpt);
                    return -1;
                } else {
                    pidBuff = atoi(argBuff);
                }
                if(strcmp(argOpt, "-p") == 0) {
                    sprintf(command, "p %d", pidBuff);
                } else if(strcmp(argOpt, "-c") == 0) {
                    sprintf(command, "c %d", pidBuff);
                } else if(strcmp(argOpt, "-s") == 0) {
                    sprintf(command, "s %d",pidBuff);
                } else {
                    //free malloc string
                    free(argBuff);
                    free(argOpt);
                    return -1;
                }
            }
        } else {
            pidBuff = atoi(argv[argc - 1]);
            sprintf(command, "c %d",pidBuff);
        }
    } else {
        //free malloc string
        free(argBuff);
        free(argOpt);
        return -1;
    }
    free(argBuff);
    free(argOpt);
    return 0;
}

char *strdup (const char *s)
{
    char *d = (char *)malloc(strlen (s) + 1);
    if (d == NULL)return NULL;
    strcpy (d,s);
    return d;
}

#endif