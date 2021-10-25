#ifndef CLIENT_H
#define CLIENT_H

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
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>

char newquery[500];

void processResponse(char * recvMsg, char * oldquery);

char *safestrtok (char *s, const char *delim, char **save_ptr)
{
    char *end;
    if (s == NULL)
        s = *save_ptr;
    if (*s == '\0') {
        *save_ptr = s;
        return NULL;
    }
    /* Scan leading delimiters.  */
    s += strspn (s, delim);
    if (*s == '\0') {
        *save_ptr = s;
        return NULL;
    }
    /* Find the end of the token.  */
    end = s + strcspn (s, delim);
    if (*end == '\0') {
        *save_ptr = end;
        return s;
    }
    /* Terminate the token and make *SAVE_PTR point past it.  */
    *end = '\0';
    *save_ptr = end + 1;
    return s;
}

bool is_dir(const char* path)
{
    struct stat buf;
    if(stat(path, &buf) < 0) {
        return false;
    }
    return S_ISDIR(buf.st_mode);
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

void * socketAndWrite(void * arg)
{
    // printf("port %d host %s newquery %s\n", port, host, newquery);

    char recvBuff[2048];
    memset(recvBuff, '0',sizeof(recvBuff));

    char newrequest[1024];
    strcpy(newrequest,"");
    snprintf(newrequest, sizeof(newrequest), "GET %s HTTP/1.x\r\nHOST: %s:%d\r\n\r\n", newquery, host, port);
    // printf("new request:\n %s",newrequest);

    int n;
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return NULL;
    }
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
        printf("\n Error : Connect Failed \n");
        return NULL;
    }

    n = write(sockfd, newrequest, sizeof(newrequest));
    if(n > 0) {
        // printf("write to server sucess\n");
    }

    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
        recvBuff[n] = 0;
    }
    printf("%s", recvBuff);
    processResponse(recvBuff, newquery);

    return NULL;
}

void processResponse(char * recvMsg, char * oldquery)
{
    // printf("%s char=%ld\n", recvMsg, strlen(recvMsg));
    char tmpName[10];
    int tmpCode;
    char content[2048];
    char contenttype[50];
    strcpy(contenttype,"");
    strcpy(tmpName,"");
    strcpy(content,"");
    sscanf(
        recvMsg,
        "HTTP/1.x %d %s\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n",
        &tmpCode,
        tmpName,
        contenttype
    );

    // Check if contenttype is empty
    if(tmpCode != 200) {
        return;
    }

    // If contenttype not empty, obtain the content from recvBuff
    char * pch;
    pch = strstr(recvMsg, "\r\n\r\n");
    substring(pch, content, 5,strlen(pch));
    // printf("content: %s\n", content);

    // If content is directory traverse directory or saving content if it is a normal file
    if(!strcmp(contenttype,"directory")) {
        // printf("\nIM A FOLDER!\n");
        char* token;
        char* rest = content;
        while ((token = safestrtok(rest, " ", &rest))) {
            strcpy(newquery,oldquery);
            strcat(newquery,"/");
            strcat(newquery,token);
            // printf("%s\n", newquery);
            pthread_t tid;
            pthread_create(&tid, NULL, socketAndWrite, NULL);
            pthread_join(tid, NULL);
        }
        // char * pch2;
        // pch2 = strtok(content, " ");
        // while(pch2 != NULL) {
        //     // printf("items %s\n", pch2);
        //     strcpy(newquery,oldquery);
        //     strcat(newquery,"/");
        //     strcat(newquery,pch2);
        //     // printf("newquery %s\n", newquery);
        //     pch2 = strtok (NULL, " ");

        //     pthread_t tid;
        //     pthread_create(&tid, NULL, socketAndWrite, NULL);
        //     pthread_join(tid, NULL);
        // }
    } else {
        // printf("IM A FILE!\n");
        // Save to output file
        char outputquery[150];
        char foldername[150];
        char outputquery2[150];
        strcpy(foldername,"");
        strcpy(outputquery,"output");
        strcat(outputquery, oldquery);
        strcpy(outputquery2,outputquery);

        // File IO
        FILE *fp;

        // printf("Whats my name : %s\n", outputquery);

        // First while get counting number, and set the last number
        char* token;
        char* rest = outputquery;

        int count = 0;
        while ((token = safestrtok(rest, "/", &rest))) {
            count++;
            // printf("%s\n", token);
        }
        int last = count;
        // printf("the last %d\n", last);

        // // Second do the operation
        char* token2;
        char* rest2 = outputquery2;
        count = 0;
        while ((token2 = safestrtok(rest2, "/", &rest2))) {
            count++;
            // printf("%s\n", token2);
            strcat(foldername,token2);
            if(count != last) {
                strcat(foldername,"/");
                // printf ("folder %s\n",foldername);
                if(!is_dir(foldername)) {
                    mkdir(foldername, 0700);
                }
            } else {
                // printf ("file %s\n",foldername);
                fp = fopen(foldername, "w");
                fprintf(fp,"%s", content);
                fclose(fp);
            }
        }
    }
    return;
}
#endif