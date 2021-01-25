#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <strings.h>

char host[100];
int port;
char processed_query[64];
struct sockaddr_in serv_addr;

#include "client.h"
//$ ./client -t QUERY_FILE_OR_DIR -h LOCALHOST -p PORT
//      0       2                       4           6

int main(int argc, char *argv[])
{
    int n;
    char recvBuff[2048];
    // struct sockaddr_in serv_addr;
    int sockfd;

    // strcpy(host, argv[4]);
    snprintf(host, sizeof(host), "%s", argv[4]);
    // printf("%s\n", host);
    port = atoi(argv[6]);
    char query[64];

    // Check whether there is a trailing slash
    strcpy(query,argv[2]);
    strcpy(processed_query,"");
    int trail = strlen(query) - 1;
    if(query[trail] == '/') {
        substring(query, processed_query, 1,trail);
        // printf("%s\n",processed_query);
    } else {
        strcpy(processed_query,query);
    }

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

    // After connection

    char requestMsg[200];
    snprintf(requestMsg, sizeof(requestMsg), "GET %s HTTP/1.x\r\nHOST: %s:%d\r\n\r\n", processed_query, host, port);
    // printf("%s", requestMsg);


    n = write(sockfd, requestMsg, sizeof(requestMsg));
    if(n > 0) {
        // printf("write to server sucess\n");
    }

    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
        recvBuff[n] = 0;
    }
    if(n < 0) {
        printf("Read error\n");
    }
    printf("%s", recvBuff);
    processResponse(recvBuff, processed_query);

    return 0;
}
