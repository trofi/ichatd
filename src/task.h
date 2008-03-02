#ifndef __TASK_H__
#define __TASK_H__

struct timed_task;
typedef void (*TASK_FUN)(void * data);

struct timed_task * task_create (long long delta, TASK_FUN fun, void * data);
void task_destroy (struct timed_task * task);
void task_run (struct timed_task * task);

struct timed_task {
    struct timed_task * next;
    long long time;   // time is stored in GetTimerMS format
    TASK_FUN fun;
    void * data;
};

#endif // __TASK_H__
