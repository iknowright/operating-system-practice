#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <errno.h>

#include "simple_pstree.h"

#define NETLINK_TEST 31
#define MAX_PAYLOAD 8096 // maximum payload size

int sock_fd;
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
struct msghdr msg;

void init_netlink(char * argument)
{
    // Create a socket
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    if(sock_fd == -1) {
        printf("error getting socket: %s", strerror(errno));
        return;
    }

    // To prepare binding
    memset(&msg,0,sizeof(msg));
    memset(&src_addr, 0, sizeof(src_addr));
    //src_address
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // self pid
    src_addr.nl_groups = 0; // multi cast
    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    //dest_address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    /* Fill the netlink message header */
    nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid(); /* self pid */
    nlh->nlmsg_flags = 0;

    /* Fill in the netlink message payload */
    strcpy(NLMSG_DATA(nlh), argument);
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    //iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
}

int main(int argc, char *argv[])
{
    char * command;
    command = (char*)malloc(100 * sizeof(char));

    if(analyzedCommand(command, argc, argv) == -1) {
        return 0;
    }
    // printf("command analyzed is \"%s\"\n",command);


    //socket
    int state;
    int state_smg = 0;

    init_netlink(command);


    // printf(" Sending message. ...\n");
    state_smg = sendmsg(sock_fd,&msg,0);
    if(state_smg == -1) {
        // printf("get error sendmsg = %s\n",strerror(errno));
    }

    memset(nlh,0,NLMSG_SPACE(MAX_PAYLOAD));

    state = recvmsg(sock_fd, &msg, 0);
    if(state<0) {
        return 0;
    }
    printf("%s",(char *) NLMSG_DATA(nlh));

    close(sock_fd);

    return 0;
}