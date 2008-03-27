#ifndef __TASK_H__
#define __TASK_H__

struct timed_task;
typedef void (*TASK_FUN)(void * data);
typedef void (*TASK_DTOR_FUN)(void * data);

struct timed_task * task_create (long long delta, TASK_FUN fun, TASK_DTOR_FUN dtor, void * data);

// selfdestructs (calls task_destroy)
void task_run (struct timed_task * task);

// do you really need it?
void task_destroy (struct timed_task * task);

struct timed_task {
    struct timed_task * next;
    unsigned long long time;   // time is stored in GetTimerMS() format
    TASK_FUN fun;
    TASK_DTOR_FUN dtor;
    void * data;
};

#endif // __TASK_H__
