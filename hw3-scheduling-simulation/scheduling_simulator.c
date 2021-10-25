#include "scheduling_simulator.h"

struct itimerval it_val;
struct taskQueue * queuing_task;
bool ctrlZFlag = false;
bool timerFlag = true;
ucontext_t scheduler,shell;
ucontext_t scheduler;
bool shellmode = true;
bool simulation_running = false;
char stack[2048*128];
struct queueNode * curr_task_node;
int cur_time_quantum = 100000;

ucontext_t idle_ctx;
char stack_idle[2048*128];
void idle(void) // idle function
{
    unsigned int a = 0;

    while (1) {
        a = a + 1;
    }
}

ucontext_t complete_ctx;
char stack_complete[2048*128];
void completion(void) // idle function
{
    while(1) {
        if(curr_task_node != NULL) {
            curr_task_node->state = TASK_TERMINATED;
        } else {
        }
        swapcontext(&complete_ctx, &scheduler);
    }
}

void hw_suspend(int msec_10)
{
    curr_task_node->msec_10 = msec_10;
    curr_task_node->state = TASK_WAITING;
    swapcontext(&(curr_task_node->ctx),&scheduler);
}

void hw_wakeup_pid(int pid)
{
    wakeNode(queuing_task, NULL, pid);
    return;
}

int hw_wakeup_taskname(char *task_name)
{
    wakeNode(queuing_task, task_name, 0);
    return 0;
}

int hw_task_create(char *task_name)
{
    char timequantum = 'S';
    char priority = 'L';
    if(!strcmp(task_name,"task1")) {
        pid++;
    } else if(!strcmp(task_name,"task2")) {
        pid++;
    } else if(!strcmp(task_name,"task3")) {
        pid++;
    } else if(!strcmp(task_name,"task4")) {
        pid++;
    } else if(!strcmp(task_name,"task5")) {
        pid++;
    } else if(!strcmp(task_name,"task6")) {
        pid++;
    } else {
        return -1;
    }
    // task create is task waiting
    struct queueNode * first = enQueue(queuing_task, pid, task_name, TASK_READY, 0, timequantum, priority, 0);
    getcontext(&(first->ctx)); //获取当前上下文
    first->ctx.uc_stack.ss_sp = first->stack;//指定栈空间
    first->ctx.uc_stack.ss_size = sizeof(first->stack);//指定栈空间大小
    first->ctx.uc_stack.ss_flags = 0;
    first->ctx.uc_link = &complete_ctx;//设置后继上下文
    makecontext(&(first->ctx), (void *)(first->func), 0);//修改上下文指向scheduling_simulator函数
    return pid; // the pid of created task name
}

// interrupt handler
void handlerCtrlZ(int sig)
{
    if(curr_task_node == NULL) {
        if(!shellmode) {
            swapcontext(&scheduler,&shell);
        }
    } else if(curr_task_node != NULL) {
        if(!shellmode) {
            swapcontext(&(curr_task_node->ctx),&shell);
        }
    }
}

void handlerTimeFrame(int sig)
{
    timerFlag = true;
    setQueueAndSuspendTime(queuing_task);
    cur_time_quantum--;
    if(curr_task_node != NULL && cur_time_quantum == 0) {
        ;
        swapcontext(&(curr_task_node->ctx),&scheduler);
    } else {
        swapcontext(&idle_ctx,&scheduler);
    }

}

int main()
{
    curr_task_node = NULL;

    // idle
    getcontext(&idle_ctx); //获取当前上下文
    idle_ctx.uc_stack.ss_sp = stack_idle;//指定栈空间
    idle_ctx.uc_stack.ss_size = sizeof(stack_idle);//指定栈空间大小
    idle_ctx.uc_stack.ss_flags = 0;
    idle_ctx.uc_link = &scheduler;//设置后继上下文
    makecontext(&idle_ctx, (void *)(idle), 0);//修改上下文指向scheduling_simulator函数

    // completion
    getcontext(&complete_ctx); //获取当前上下文
    complete_ctx.uc_stack.ss_sp = stack_complete;//指定栈空间
    complete_ctx.uc_stack.ss_size = sizeof(stack_complete);//指定栈空间大小
    complete_ctx.uc_stack.ss_flags = 0;
    complete_ctx.uc_link = 0;//设置后继上下文
    makecontext(&complete_ctx, (void *)(completion), 0);//修改上下文指向scheduling_simulator函数

    // initialize task queues
    queuing_task = createQueue();

    // signal handle ctrl + z
    signal(SIGTSTP,handlerCtrlZ);
    // signal handle alarm clock
    signal(SIGALRM,handlerTimeFrame);

    // scheduling_simulator();
    shell_loop();
    return 0;
}

void shell_loop()
{
    printf("---------Shell Mode---------\n");
    // read a line
    char *line;

    do {
        printf("$ ");
        line = shell_read_line();
        struct LineArgument *lineArg = shell_split_line(line);
        execution(lineArg);
    } while(1);
}

char *shell_read_line()
{
    char* line = NULL;
    size_t size = 0;
    getline(&line, &size, stdin);
    return line;
}

struct LineArgument* shell_split_line(char * line)
{
    struct LineArgument *lineArg = (struct LineArgument*)malloc(sizeof(struct LineArgument));
    int buffsize = TOKEN_BUFSIZE;
    int argc = 0;
    char **tokens = malloc(buffsize * sizeof(char *));
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
    int argc = lineArg->argc;
    if(!strcmp(args[0], "add")) {
        // add TASK_NAME -t TIME_QUANTUM –p PRIORITY
        // 0   1         2  3            4  5
        char timequantum = 'S';
        char priority = 'L';
        if(argc == 6) {
            timequantum = args[3][0];
            priority = args[5][0];
        } else if(argc == 4) {
            if(!strcmp(args[2],"-t")) {
                timequantum = args[3][0];
            } else if(!strcmp(args[2],"-p")) {
                priority = args[3][0];
            }
        }
        int pidinput = hw_task_create(args[1]);
        if(pidinput == -1) {
            printf("No task name %s\n", args[1]);
        } else {
            queuing_task->rear->time_quantum = timequantum;
            queuing_task->rear->priority = priority;
        }
    } else if (!strcmp(args[0], "remove")) {
        // remove pid
        // 0      1
        if(argc == 2) {
            if(curr_task_node == NULL) {
                removeNode(queuing_task, atoi(args[1]));
            } else if(atoi(args[1]) ==  curr_task_node->pid) {
                curr_task_node = NULL;
            }
        }
        // popNode(queuing_task);
    } else if (!strcmp(args[0], "ps")) {
        if(curr_task_node != NULL) {
            printf("%d %s %s %d %c %c\n",
                   curr_task_node->pid,
                   curr_task_node->name,
                   "TASK_RUNNING",
                   curr_task_node->queuing_time,
                   curr_task_node->time_quantum,
                   curr_task_node->priority
                  );
        }
        printAll(queuing_task);
    } else if (!strcmp(args[0], "start")) {
        it_val.it_value.tv_sec = INTERVAL/1000;
        it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;
        it_val.it_interval = it_val.it_value;
        setitimer(ITIMER_REAL, &it_val, NULL);
        if(simulation_running == false) {
            getcontext(&scheduler); //获取当前上下文
            scheduler.uc_stack.ss_sp = stack;//指定栈空间
            scheduler.uc_stack.ss_size = sizeof(stack);//指定栈空间大小
            scheduler.uc_stack.ss_flags = 0;
            scheduler.uc_link = &shell;//设置后继上下文
            makecontext(&scheduler, (void *)scheduling_simulator, 0);//修改上下文指向scheduling_simulator函数
        }
        if(curr_task_node == NULL) {
            // printf("from shell to scheduler\n");
            shellmode = false;
            swapcontext(&shell,&scheduler);//切换到scheduler上下文，保存当前上下文到main
        } else {
            // printf("from shell to function\n");
            shellmode = false;
            swapcontext(&shell,&(curr_task_node->ctx));
        }
        // puts("back to shell");//如果设置了后继上下文，func1函数指向完后会返回此处
        shellmode = true;
        it_val.it_value.tv_sec = 0;
        it_val.it_value.tv_usec = 0;
        it_val.it_interval = it_val.it_value;
        setitimer(ITIMER_REAL, &it_val, NULL);
    }
    return;
}

void scheduling_simulator(void)
{
    simulation_running = true;
    printf("---------Simulation Mode---------\n");
    while(hasWork(queuing_task)) {
        curr_task_node = (struct queueNode*)malloc(sizeof(struct queueNode));
        curr_task_node = popNode(queuing_task);
        if(curr_task_node != NULL) {
            if(curr_task_node->queuing_time == 'L') {
                cur_time_quantum = 2;
            } else {
                cur_time_quantum = 1;
            }
            curr_task_node->state = TASK_READY;
            swapcontext(&scheduler,&(curr_task_node->ctx));
            if(curr_task_node != NULL) {
                struct queueNode * first = enQueue(
                                               queuing_task,
                                               curr_task_node->pid,
                                               curr_task_node->name,
                                               curr_task_node->state,
                                               curr_task_node->queuing_time,
                                               curr_task_node->time_quantum,
                                               curr_task_node->priority,
                                               curr_task_node->msec_10
                                           );
                first->ctx = curr_task_node->ctx;
                curr_task_node = NULL;
            }
        } else {
            cur_time_quantum = 1;
            swapcontext(&scheduler,&idle_ctx);
        }
        timerFlag = false;
    }
    simulation_running = false;
}

struct queueNode* newNode (int pid, char * name, enum TASK_STATE state, int queuing_time, char time_quantum, char priority, int msec_10)
{
    struct queueNode *tmp = (struct queueNode*)malloc(sizeof(struct queueNode));
    switch(state) {
    case TASK_READY: {
        char * sname = "TASK_READY";
        tmp->state_name = sname;
        break;
    }
    case TASK_TERMINATED: {
        char * sname = "TASK_TERMINATED";
        tmp->state_name = sname;
        break;
    }
    case TASK_RUNNING: {
        char * sname = "TASK_RUNNING";
        tmp->state_name = sname;
        break;
    }
    case TASK_WAITING: {
        char * sname = "TASK_WAITING";
        tmp->state_name = sname;
        break;
    }
    }
    if(!strcmp(name,"task1")) {
        tmp -> func = task1;
    } else if(!strcmp(name,"task2")) {
        tmp -> func = task2;
    } else if(!strcmp(name,"task3")) {
        tmp -> func = task3;
    } else if(!strcmp(name,"task4")) {
        tmp -> func = task4;
    } else if(!strcmp(name,"task5")) {
        tmp -> func = task5;
    } else if(!strcmp(name,"task6")) {
        tmp -> func = task6;
    }
    tmp -> pid = pid;
    tmp -> name = name;
    tmp -> state = state;
    tmp -> queuing_time = queuing_time;
    tmp -> time_quantum = time_quantum;
    tmp -> priority = priority;
    tmp -> msec_10 = msec_10;
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

struct queueNode * enQueue(struct taskQueue *q, int pid, char * name, enum TASK_STATE state, int queuing_time,  char timequantum, char priority, int msec_10)
{
    struct queueNode *tmp = newNode(pid, name, state, queuing_time, timequantum, priority, msec_10);
    if(q -> rear == NULL) {
        q->front = tmp;
        q->rear = tmp;
        return tmp;
    }
    q->rear->next = tmp;
    q->rear = tmp;
    return tmp;
}

void printAll(struct taskQueue * q)
{
    // printf("print all the queues\n");
    if(q->front == NULL) {
        // printf("nothong to remove\n");
        return;
    }
    struct queueNode *tmp = q->front;
    while(tmp != NULL) {
        printf("%d %s %s %d %c %c\n",
               tmp->pid,
               tmp->name,
               tmp->state_name,
               tmp->queuing_time,
               tmp->time_quantum,
               tmp->priority
              );
        tmp = tmp->next;
    }
    return;
}

void removeNode(struct taskQueue * q, int pid)
{
    if(q->front == NULL) {
        if(q->rear == NULL) {
            return;
        }
        return;
    }

    struct queueNode *tmp = q->front, *prev;

    if(q->front == q->rear && q->front->pid == pid) {
        q->front = q->rear = NULL;
        return;
    }
    if (tmp != NULL && tmp->pid == pid) {
        q->front = tmp->next;   // Changed head
        // free(tmp);        // free old head
        return;
    }

    while (tmp != NULL && tmp->pid != pid) {
        prev = tmp;
        tmp = tmp->next;
    }
    if(tmp == q->rear) {
        q->rear = prev;
        q->rear->next = NULL;
    }

    if(tmp == NULL) return;

    free(tmp);

    return;
}

struct queueNode *popNode(struct taskQueue * q)
{
    if(q->front == NULL) {
        if(q->rear == NULL) {
            return NULL;
        }
        return NULL;
    }

    struct queueNode *tmp = q->front, *prev;
    if(q->front == q->rear && tmp->state == TASK_READY) {
        q->front = q->rear = NULL;
        return tmp;
    }
    if (tmp != NULL && tmp->priority == 'H') {
        if(tmp->state == TASK_READY) {
            q->front = tmp->next;
            return tmp;
        }
    }
    while (tmp != NULL && (tmp->priority != 'H' || tmp->state != TASK_READY)) {
        prev = tmp;
        tmp = tmp->next;
    }
    if(tmp != NULL  && tmp->state == TASK_READY) {
        if(tmp == q->rear) {
            q->rear = prev;
            q->rear->next = NULL;
            return tmp;
        }
        prev->next = tmp->next;
        return tmp;
    }
    tmp = q->front;
    if (tmp != NULL && tmp->priority == 'L') {
        if(tmp->state == TASK_READY) {
            q->front = tmp->next;
            return tmp;
        }
    }
    while (tmp != NULL && (tmp->priority != 'L' || tmp->state != TASK_READY)) {
        prev = tmp;
        tmp = tmp->next;
    }
    if(tmp != NULL && tmp->state == TASK_READY) {
        if(tmp == q->rear) {
            q->rear = prev;
            q->rear->next = NULL;
            return tmp;
        }
        prev->next = tmp->next;
        return tmp;
    }
    return NULL;
}

void wakeNode(struct taskQueue * q, char * name, int pid)
{
    if(q->front == NULL) {
        return;
    }
    struct queueNode *tmp = q->front;
    while(tmp != q->rear->next) {
        if(pid == 0) {
            char tmpname[10];
            strcpy(tmpname, tmp->name);
            if(!strcmp(tmp->name,name) && tmp->state == TASK_WAITING) {
                char * state_name = "TASK_READY";
                tmp->msec_10 = 0;
                tmp->state_name = state_name;
                tmp->state = TASK_READY;
            }
        } else if(name == NULL) {
            if(tmp->pid == pid && tmp->state == TASK_WAITING) {
                char * state_name = "TASK_READY";
                tmp->msec_10 = 0;
                tmp->state_name = state_name;
                tmp->state = TASK_READY;
                return;
            }
        }
        tmp = tmp->next;
    }
}

void setQueueAndSuspendTime(struct taskQueue * q)
{
    if(q->front == NULL) {
        return;
    }
    struct queueNode *tmp = q->front;
    while(tmp != q->rear->next) {
        if(tmp->state != TASK_TERMINATED) {
            tmp->queuing_time += 10;
            if(tmp->state == TASK_WAITING && tmp->msec_10 != 0) {
                tmp->msec_10--;
            } else if (tmp->state == TASK_WAITING && tmp->msec_10 == 0) {
                tmp->state = TASK_READY;
                char * state_name = "TASK_READY";
                tmp->state_name = state_name;
            }
        }
        tmp = tmp->next;
    }
}

bool hasWork(struct taskQueue * q)
{
    if(q->front == NULL) {
        return false;
    }
    struct queueNode *tmp = q->front;
    while(tmp != q->rear->next) {
        if(tmp->state == TASK_READY || tmp->state == TASK_WAITING) {
            return true;
        }
        tmp = tmp->next;
    }
    return false;
}
