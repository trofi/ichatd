#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/select.h>

#include "poller.h"
#include "list.h"
#include "server.h"
#include "client.h"

#include "log.h"
#include "task.h"
#include "misc.h"

#include "debug.h"

enum POLL_RESULT
server_poll (struct server * server)
{
    struct client * clnt = 0; // unneeded init

    for (;;)
    {
        // TODO: add array of pointers to client structs (fast search, if we need 'em)
        // TODO: remove this perf lossy cycle
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
                POLL_DEBUG ("%s: try remove client[fd=%d]", __func__, clnt->fd);
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
    POLL_DEBUG ("%s: fill", __func__);
    list_for_each(clnt, server->clist)
    {
        assert (clnt->fd >= 0);

        int is_set = 0;
        if (clnt->op.can_read
            && clnt->op.can_read(server, clnt)
            && clnt->op.read)
        {
            FD_SET(clnt->fd, &rd);
            is_set = 1;
            POLL_DEBUG ("%s: set [R]%d", __func__, clnt->fd);
        }
        
        if (clnt->op.can_write
            && clnt->op.can_write(server, clnt)
            && clnt->op.write)
        {
            FD_SET(clnt->fd, &wr);
            is_set = 1;
            POLL_DEBUG ("%s: set [W]%d", __func__, clnt->fd);
        }
        if (clnt->op.error)
        {
            FD_SET(clnt->fd, &ex);
            is_set = 1;
            POLL_DEBUG ("%s: set [E]%d", __func__, clnt->fd);
        }
        if  (is_set)
            max_fd = (max_fd < clnt->fd) ? clnt->fd : max_fd;
    }
    POLL_DEBUG ("%s: fill done", __func__);

    struct timeval tv = { 0, 0 };
    struct timeval * ptv = NULL;
    
    if (server->task_queue) // yet tasks, which can be timed out
    {
        ptv = &tv;
        unsigned long long curr  = GetTimerMS();
        unsigned long long sched = server->task_queue->time;
        if (sched <= curr)
        {
            POLL_DEBUG ("%s: we are here: late = %d ms", __func__, (long)(curr - sched));
            tv.tv_usec = 10 * 1000; // let's wait a little (10ms)
        }
        else
        {
            POLL_DEBUG ("%s: set timeout to %ld ms", __func__, (long)(sched - curr));
            tv.tv_usec = (sched - curr)*1000; // TODO: explicit check
        }
    }

    int result = select (max_fd + 1, &rd, &wr, &ex, ptv);

    if (result == -1 && errno != EINTR)
    {
        WARN ("%s: %s", __func__, strerror (errno));
        switch (errno)
        {
            case EBADF:
                WARN ("%s: %s", __func__, "hmm.. EBADF happened");
            case EINTR:
            default:
                // all ok. ignore and quit
                return POLL_OK;
        }
        return POLL_ERROR;
    }
    POLL_DEBUG ("%s: check", __func__);
    list_for_each(clnt, server->clist)
    {
        if (FD_ISSET(clnt->fd, &rd)
            && !clnt->corrupt /* MUST be always true */ )
        {
            clnt->op.read (server, clnt);
            POLL_DEBUG ("%s: do [R]%d", __func__, clnt->fd);
        }
        if (FD_ISSET(clnt->fd, &wr)
            && !clnt->corrupt /* can be triggered by prev ops */)
        {
            clnt->op.write (server, clnt);
            POLL_DEBUG ("%s: do [W]%d", __func__, clnt->fd);
        }
        if (FD_ISSET(clnt->fd, &ex)
            && !clnt->corrupt /* can be triggered by prev ops */)
        {
            clnt->op.error (server, clnt);
            POLL_DEBUG ("%s: do [E]%d", __func__, clnt->fd);
        }
    }

    return POLL_OK;
}
