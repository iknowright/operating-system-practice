#ifndef KSIMPLE_PSTREE
#define KSIMPLE_PSTREE

#include <linux/list.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/list.h>

#define MAX_MSGSIZE 8096

void doParent(char* msg, int target_pid, char * tmp);
void doSibling(char * msg, int target_pid, char * tmp);
void doChildrenRecursion(char * msg, int target_pid, int level, char * tmp);

int i, j, k;
int parent_pid_holder[100];
int parentNum;

void getPstree(char * msg, char * command, char * tmp)
{
    printk("net_link: GetPstree received command \"%s\"\n", command);
    int target_pid = 0;
    int command_length = strlen(command);
    int i, power[command_length];
    power[0] = 1;
    for(i = 1; i < command_length; i++) {
        power[i] = power[i -1] * 10;
    }
    for(i = 2; i < command_length; i++) {
        //printk("net_link: %d\n", command[i]);
        target_pid += (command[i] - '0') * power[command_length - i - 1];
    }
    // Target PID Obtained
    printk("net_link:pid %d\n", target_pid);
    printk("net_link:opt char %c\n", command[0]);

    //Do things on option and pid
    strcpy(msg, "");
    if(command[0] == 'p') {
        doParent(msg, target_pid, tmp);
        return;
    }
    if(command[0] == 'c') {
        doChildrenRecursion(msg, target_pid, 0, tmp);
        return;
    }
    if(command[0] == 's') {
        doSibling(msg, target_pid, tmp);
        return;
    }
    return;
}

void doParent(char * msg, int target_pid, char * tmp)
{
    struct pid *pid_struct;
    struct task_struct *task;
    pid_struct = find_get_pid(target_pid);
    if(pid_struct == NULL) {
        strcpy(msg, "");
        snprintf(msg, MAX_MSGSIZE,"Pid not valid");
        printk("net_link:pid not valid\n");
    } else {
        task = pid_task(pid_struct,PIDTYPE_PID);
        printk("net_link: name is %s\n", task -> comm);
        char application[100];
        sprintf(application, "%s(%d)\n",task -> comm,task -> pid);

        i = 0;
        while(find_get_pid(task -> parent -> pid) != NULL) {
            task = task -> parent;
            parent_pid_holder[i] = task->pid;
            printk("net_link: name is %s\n", task -> comm);
            printk("net_link: pid is %d\n", task -> pid);
            printk("net_link: layer is %d\n", i);
            i++;
        }
        parentNum = i;
        for(j = 1; j <= parentNum + 1; j++) {
            //getting tabs
            for(k = 0; k < j - 1; k++) {
                snprintf(msg, MAX_MSGSIZE, "%s    ", tmp);
                strcpy(tmp,msg);
            }
            if(j == parentNum + 1) {
                snprintf(msg, MAX_MSGSIZE, "%s%s", tmp, application);
                strcpy(tmp,tmp);
            } else {
                //re-assign parent pid and find it
                pid_struct = find_get_pid(parent_pid_holder[parentNum - j]);
                task = pid_task(pid_struct,PIDTYPE_PID);
                snprintf(msg, MAX_MSGSIZE, "%s%s(%d)\n", tmp, task->comm, parent_pid_holder[parentNum - j]);
                strcpy(tmp,msg);
            }
        }
    }
    return;
}

void doSibling(char * msg, int target_pid, char * tmp)
{
    struct pid *pid_struct;
    struct task_struct *task, *sibling_ts;
    struct list_head *pos, sibling_lh;

    pid_struct = find_get_pid(target_pid);
    task = pid_task(pid_struct, PIDTYPE_PID);
    if(task == NULL) {
        strcpy(msg, "");
        printk("net_link:pid not valid\n");
        return;
    }
    if(list_empty(&(task->sibling))) {
        strcpy(msg, "");
        return;
    }
    sibling_lh = task->sibling;
    //for sibling
    strcpy(tmp,"");
    list_for_each(pos, &sibling_lh) {
        sibling_ts = list_entry(pos, struct task_struct, sibling);
        if(task->pid == sibling_ts->pid) return;
        if(sibling_ts->pid != 0 && find_get_pid(sibling_ts->pid) != NULL) {
            printk("net_link: name = %s pid= %d\n", sibling_ts->comm, sibling_ts->pid);
            snprintf(msg, MAX_MSGSIZE,"%s%s(%d)\n", tmp, sibling_ts->comm, sibling_ts->pid);
            strcpy(tmp,msg);
        }
    }
    return;
}

void doChildrenRecursion(char * msg, int target_pid, int level, char * tmp)
{
    struct pid *pid_struct;
    struct task_struct *task, *children_ts;
    struct list_head *pos, children_lh;
    //only test first layer validity
    if(level == 0) {
        pid_struct = find_get_pid(target_pid);
        if(pid_struct == NULL) {
            printk("net_link: Pid not valid\n");
            snprintf(msg, MAX_MSGSIZE,"Pid not valid");
            return;
        } else {
            task = pid_task(pid_struct, PIDTYPE_PID);
            //Print yourself
            snprintf(msg, MAX_MSGSIZE,"%s%s(%d)\n", tmp, task->comm, task->pid);
            strcpy(tmp,msg);
            printk("\n\n\n\nnet_link:Answer %s\n", tmp);
        }
    }

    pid_struct = find_get_pid(target_pid);
    task = pid_task(pid_struct, PIDTYPE_PID);
    if(list_empty(&(task->children))) {
        return;
    } else {
        printk("net_link: ---->Have children\n");
        children_lh = task->children;
        list_for_each(pos, &children_lh) {
            printk("net_link: [Level%d]\n", level);
            children_ts = list_entry(pos, struct task_struct, sibling);
            if(find_get_pid(children_ts->pid) == NULL || children_ts->pid  == 0) {
                return;
            }
            for(i = 0; i < level + 1; i++) {
                snprintf(msg, MAX_MSGSIZE, "%s    ", tmp);
                strcpy(tmp,msg);
            }
            snprintf(msg, MAX_MSGSIZE,"%s%s(%d)\n", tmp, children_ts->comm, children_ts->pid);
            strcpy(tmp,msg);
            printk("net_link: %s(%d)\n", children_ts->comm, children_ts->pid);
            doChildrenRecursion(msg, children_ts->pid, level + 1, tmp);
        }
        return;
    }
    return;
}
#endif