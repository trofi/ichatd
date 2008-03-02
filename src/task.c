#include <stdlib.h>
#include <assert.h>

#include "task.h"
#include "misc.h"

struct timed_task *
task_alloc()
{
    struct timed_task * task = (struct timed_task *)malloc (sizeof (struct timed_task));
    return task;
}

struct timed_task *
task_create (long long delta, TASK_FUN fun, void * data)
{
    struct timed_task * task = task_alloc ();
    if (!task)
        return 0;
    task->next = 0;
    task->time = GetTimerMS() + delta;
    task->fun  = fun;
    task->data = data;
    return task;
}

void
task_destroy (struct timed_task * task)
{
    if (!task)
        return;
    free (task->data);
    free (task);
}

void
task_run (struct timed_task * task)
{
    assert (task);
    task->fun (task->data);
}
