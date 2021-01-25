#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <string.h>
#include "status.h"
#include <pthread.h>
#include "server.h"
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
// $ ./server –r root -p port -n thread_number
//      0               2       4       6

//requestMsg
// HTTP/1.x 200 OK\r\nContent-Type: text/html\r\nServer: httpserver/1.x\r\n\r\nCONTENT
//     statusId status              type                                        content
int listenfd;
int connfd;

int threadcounter;

struct taskQueue * q;
pthread_mutex_t lock;
int count;
int debugNum = 0;

char sendBuff[1024];
char root[100];
char requestBuff[1024];
char request[5000];
// Process Request Section
void *doRequest(void* arg)
{
    int idflag = 0;
    int id;
    while(1) {
        if(idflag == 0) {
            idflag = 1;
            id = ++threadcounter;
            // printf("Thread id creation %d\n", id);
        }
        pthread_mutex_trylock (&lock);
        struct queueNode *node = deQueue(q);
        if(node != NULL) {
            // printf("The thread with the job id %d\n", id);
            response(node->request, root);
            // printf("Respond ended\n");
        }
        pthread_mutex_unlock (&lock);
    }
    return NULL;
}

void sigint_handler(int sig)
{
    // printf("%d %d\n", listenfd, connfd);
    close(listenfd);
    close(connfd);
    // printf("^c\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    // TaskQueue
    q = createQueue();

    // int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr, cli_addr;

    int port = atoi(argv[4]);
    int thread_number = atoi(argv[6]);
    strcpy(root, argv[2]);
    // printf("%s\n", root);

    // Things Socket Does
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // fcntl(listenfd, F_SETFL, O_NONBLOCK);

    // Instantly reuse port after server close
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));
    memset(requestBuff, '0', sizeof(requestBuff));
    memset(request, '0', sizeof(request));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, 10);

    // Initialize mutex lock
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    // Create Threads
    int i;
    pthread_t tid[thread_number];
    for(i = 0; i < thread_number; i++) {
        pthread_create(&tid[i], NULL, doRequest, NULL);
    }

    // Socket Keep Reading Requests
    while(1) {
        // printf("%d\n", ++debugNum);
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr,&clilen);
        // printf("connfd %d\n", connfd);

        read(connfd, requestBuff, sizeof(requestBuff)-1);

        sprintf(request,"%d %s", connfd, requestBuff);

        enQueue(q, request);

        // printf("After enqueue\n");

        // sleep(1);
    }
}