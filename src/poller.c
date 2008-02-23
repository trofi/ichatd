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

    // TODO: add sane timeout computation (support timer scheduled jobs)
    int result = select (max_fd + 1, &rd, &wr, &ex, NULL);

    if ((result == -1 && errno == EINTR)
        || result == 0)
        return POLL_IDLE;

    if (result == -1)
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
    POLL_DEBUG ("poller: check done");
    return POLL_OK;
}
