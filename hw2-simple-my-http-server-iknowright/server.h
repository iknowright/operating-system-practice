#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "status.h"
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

bool is_dir(const char* path)
{
    struct stat buf;
    if(stat(path, &buf) < 0) {
        return false;
    }
    return S_ISDIR(buf.st_mode);
}

// Response format:
// HTTP/1.x 200 OK\r\nContent-Type: text/html\r\nServer: httpserver/1.x\r\n\r\nCONTENT

void response(char * request, char * root)
{
    FILE *fp;

    char method[10];
    char query[64];
    char setQuery[128];
    char content[2048];
    char tmp[2048];
    char response[4096];
    int socketfd = 0;
    strcpy(tmp,"");
    strcpy(content,"");
    strcpy(setQuery,"");
    strcpy(method,"");
    strcpy(query,"");
    strcpy(response,"");

    sscanf (request, "%d %s %s HTTP/1.x", &socketfd, method, query);
    // printf("%d %s %s %s\n", socketfd, method, query, root);

    // Check if the request method is restricted to GET
    if(strcmp(method, "GET") != 0) {
        sprintf(
            response,
            "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",
            status_code[METHOD_NOT_ALLOWED],
            status_name[METHOD_NOT_ALLOWED],
            "",
            ""
        );
        write(socketfd, response, strlen(response));
        close(socketfd);
        return;
    }

    // Check if request file or directory doest start with /
    if(query[0] != '/') {
        sprintf(
            response,
            "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",
            status_code[BAD_REQUEST],
            status_name[BAD_REQUEST],
            "",
            ""
        );
        write(socketfd, response, strlen(response));
        close(socketfd);
        return;
    }

    // Get the query file/directory
    strcpy(setQuery, root);
    strcat(setQuery, query);
    // printf("%s\n", setQuery);

    if(is_dir(setQuery)) {
        DIR *d;
        struct dirent *dir;
        d = opendir(setQuery);
        int first = 1;
        while ((dir = readdir(d)) != NULL) {
            if(strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                if(first == 1) {
                    strcat(content, dir->d_name);
                    first = 0;
                } else {
                    strcat(content, " ");
                    strcat(content, dir->d_name);
                }
            }
        }
        strcat(content, "\n");
        sprintf(
            response,
            "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",
            status_code[OK],
            status_name[OK],
            "directory",
            content
        );
        write(socketfd, response, strlen(response));
        close(socketfd);
        return;
    }

    // Read the file in advance
    fp = fopen(setQuery, "r");

    // Check if it is a file
    if(fp == NULL) {
        // If not, check if it is a directory
        // printf("Not file or folder\n");
        sprintf(
            response,
            "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",
            status_code[NOT_FOUND],
            status_name[NOT_FOUND],
            "",
            ""
        );
        write(socketfd, response, strlen(response));
        close(socketfd);
        return;
    } else {
        // printf("File exists\n");

        // Collect content
        char c;
        c = fgetc(fp);
        while(c != EOF) {
            sprintf(tmp,"%s%c",content, c);
            strcpy(content,tmp);
            c = fgetc(fp);
        }

        // Get extension name or none
        char * extname = strrchr(query, '.');
        if(!extname || extname == query) {
            strcpy(extname, "");
        }
        // printf("extension: %s\n", extname);

        // Catch index of target extension in struct array
        int ext_num = sizeof(extensions) / sizeof(extensions[0]) - 1;
        int i;
        char ext_tmp[10];
        int index = -1;
        strcpy(ext_tmp, ".");
        for(i = 0; i < ext_num; i++) {
            strcpy(ext_tmp, ".");
            strcat(ext_tmp, extensions[i].ext);
            // printf("%s\n", ext_tmp);
            if(strcmp(extname, ext_tmp) == 0) {
                index = i;
            }
        }
        // Check if it is not supported
        if(index < 0) {
            sprintf(
                response,
                "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",
                status_code[UNSUPPORT_MEDIA_TYPE],
                status_name[UNSUPPORT_MEDIA_TYPE],
                "",
                ""
            );
            write(socketfd, response, strlen(response));
            close(socketfd);
            return;
        }

        // Send response back
        sprintf(
            response,
            "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s",
            status_code[OK],
            status_name[OK],
            extensions[index].mime_type,
            content
        );
        write(socketfd, response, strlen(response));
        close(socketfd);
    }
    fclose(fp);
    return;
}

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;

    // Stroing start time
    clock_t start_time = clock();

    // looping till required time is not acheived
    while (clock() < start_time + milli_seconds)
        ;
}

// Task Queue Section
struct queueNode {
    char * request;
    struct queueNode *next;
};

struct taskQueue {
    struct queueNode *front, *rear;
};

struct queueNode* newNode (char * msg)
{
    struct queueNode *tmp = (struct queueNode*)malloc(sizeof(struct queueNode));
    tmp -> request = msg;
    tmp -> next = NULL;
    return tmp;
}

struct taskQueue *createQueue()
{
    struct taskQueue *q = (struct taskQueue*)malloc(sizeof(struct taskQueue));
    q -> front = NULL;
    q -> rear = NULL;
    return q;
}

void enQueue(struct taskQueue *q, char * msg)
{
    struct queueNode *tmp = newNode(msg);

    if(q -> rear == NULL) {
        q->front = tmp;
        q->rear = tmp;
        return;
    }
    q->rear->next = tmp;
    q->rear = tmp;
    return;
}

struct queueNode *deQueue(struct taskQueue * q)
{
    if(q->front == NULL) {
        return NULL;
    }

    struct queueNode *tmp = q->front;
    q->front = q->front->next;

    if(q->front == NULL) {
        q->rear = NULL;
    }

    return tmp;
}

int printQueue(struct taskQueue * q, int flag)
{
    int count = 0;
    if(q->front == NULL) {
        return 0;
    } else {
        count ++;
        if(flag) {
            printf("%s",q->front->request);
        }
    }
    struct queueNode * tmp = q->front;
    while(tmp->next != NULL) {
        if(flag) {
            printf("%s",tmp->request);
        }
        count ++;
        tmp = tmp->next;
    }
    return count;
}

#endif
