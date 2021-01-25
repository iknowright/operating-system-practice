#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <ucontext.h>

// Start of my insertion
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

#define TOKEN_BUFSIZE 500
#define INTERVAL 10
#define  DEFAULT_STACK_SIZE 128 * 1024
int pid;

// End of my insertion
#include "task.h"

enum TASK_STATE {
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING,
    TASK_TERMINATED
};

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);

// Start of my code
typedef void (*Fun)(void);

struct LineArgument {
    char **args;
    int argc;
};

void shell_loop();
char *shell_read_line();
struct LineArgument* shell_split_line(char * line);
void execution(struct LineArgument *lineArg);
void scheduling_simulator();

// queue implementation
struct queueNode {
    ucontext_t ctx;
    Fun func;
    enum TASK_STATE state;
    char * name;
    char * state_name;
    char time_quantum;
    int pid;
    int queuing_time;
    char priority;
    int msec_10;
    char stack[DEFAULT_STACK_SIZE];
    struct queueNode *next;
};

struct taskQueue {
    struct queueNode *front, *rear;
};

extern struct queueNode* newNode(int pid, char * name, enum TASK_STATE state, int queuing_time, char time_quantum, char priority, int msec_10);
extern struct taskQueue *createQueue();
extern struct queueNode* enQueue(struct taskQueue *q, int pid, char * name, enum TASK_STATE state, int queuing_time,  char timequantum, char priority, int msec_10);
extern struct queueNode *deQueue(struct taskQueue * q);
extern void printAll(struct taskQueue * q);
extern struct queueNode *popNode(struct taskQueue * q);
extern void removeNode(struct taskQueue * q, int pid);

extern void wakeNode(struct taskQueue * q, char * name, int pid);
extern void setQueueAndSuspendTime(struct taskQueue * q);
extern void suspendNode(struct taskQueue * q, int pid, int msec_10);
extern bool hasWork(struct taskQueue * q);

// End of my code

#endif
