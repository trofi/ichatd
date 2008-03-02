#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#include "poller.h"
#include "list.h"
#include "server.h"
#include "client.h"

#include "log.h"
#include "task.h"
#include "misc.h"

//#define ENABLE_POLL_DEBUG 1
#ifdef ENABLE_POLL_DEBUG
#    define POLL_DEBUG(fmt, args...) DEBUG(fmt, ##args)
#else // ENABLE_POLL_DEBUG
#    define POLL_DEBUG(fmt, args...) do {} while (0);
#endif // ENABLE_POLL_DEBUG


enum POLL_RESULT
server_poll (struct server * server)
{
    DEBUG (__func__);

    struct client * clnt = 0; // unneeded init
    
    for (;;)
    {
        // TODO: perf lossy cycle
        // TODO: redo lame stupid structure
        // TODO: all these loops are iterating
        //       thru all clients - it's not acceptable
        //       i can reduce number of clients building
        //       shorted (active) queue
        // TODO: lame list_for_each API: broken deletion :]

        list_for_each(clnt, server->clist)
        {
            if (clnt->corrupt)
            {
                POLL_DEBUG ("poller: try remove %p client", clnt);
                server_remove_client (server, clnt);
                client_destroy (clnt);
                // we modified element from iterated structure - very bad
                break;
            }
        }
        // no corrupted clients
        break;
    }

    int max_fd = 0;
    fd_set rd, wr, ex;
    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&ex);
    POLL_DEBUG ("poller: fill");
    list_for_each(clnt, server->clist)
    {
        int is_set = 0;
        if (clnt->op.can_read
            && clnt->op.can_read(server, clnt)
            && clnt->op.read)
        {
            FD_SET(clnt->fd, &rd);
            is_set = 1;
            POLL_DEBUG ("poller: set [R]%d", clnt->fd);
        }
        
        if (clnt->op.can_write
            && clnt->op.can_write(server, clnt)
            && clnt->op.write)
        {
            FD_SET(clnt->fd, &wr);
            is_set = 1;
            POLL_DEBUG ("poller: set [W]%d", clnt->fd);
        }
        if (clnt->op.error)
        {
            FD_SET(clnt->fd, &ex);
            is_set = 1;
            POLL_DEBUG ("poller: set [E]%d", clnt->fd);
        }
        if  (is_set)
            max_fd = (max_fd < clnt->fd) ? clnt->fd : max_fd;
    }
    POLL_DEBUG ("poller: fill done");

    struct timeval tv = { 0, 0 };
    struct timeval * ptv = NULL;
    
    if (server->task) // yet tasks, which can be timed out
    {
        ptv = &tv;
        long long curr  = GetTimerMS();
        long long sched = server->task->time;
        if (sched <= curr)
        {
            POLL_DEBUG ("poller: we are here: late = %d us", (long)(curr - sched)*1000);
            tv.tv_usec = 10 * 1000; // let's wait a little (10ms)
        }
        else
        {
            POLL_DEBUG ("poller: set timeout to %ld us", (long)((sched - curr)*1000));
            tv.tv_usec = (sched - curr)*1000; // TODO: explicit check
        }
    }

    int result = select (max_fd + 1, &rd, &wr, &ex, ptv);

    if (result == -1 && errno != EINTR)
    {
        DEBUG ("poll error: %s", strerror (errno));
        return POLL_ERROR;
    }
    POLL_DEBUG ("poller: check");
    list_for_each(clnt, server->clist)
    {
        if (FD_ISSET(clnt->fd, &rd))
        {
            clnt->op.read (server, clnt);
            POLL_DEBUG ("poller: do [R]%d", clnt->fd);
        }
        if (FD_ISSET(clnt->fd, &wr))
        {
            clnt->op.write (server, clnt);
            POLL_DEBUG ("poller: do [W]%d", clnt->fd);
        }
        if (FD_ISSET(clnt->fd, &ex))
        {
            clnt->op.error (server, clnt);
            POLL_DEBUG ("poller: do [E]%d", clnt->fd);
        }
    }
    POLL_DEBUG ("poller: timeouts check");
    while (server->task
            && server->task->time <= GetTimerMS())
    {
        struct timed_task * task = server_pop_task (server);

        POLL_DEBUG ("poller: popping + execing task %p", task);

        task_run (task);
        task_destroy (task);
    }
    POLL_DEBUG ("poller: check done");
    return POLL_OK;
}
