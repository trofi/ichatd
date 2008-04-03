#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ichat_s2s_link.h"
#include "ichat_s2s_client.h"

#include "task.h"
#include "log.h"
#include "server.h"
#include "config.h"
#include "misc.h"

int
start_ichat_s2s_link (const struct s2s_block * b)
{
    // TODO: register reconnection task at remote server shutdown
    NOTE ("LINK to %s:%d", b->host, b->port);
    int remote = IC_nonblock_connect (b->host, b->port);
    if (remote == -1)
    {
        // we do NOT exit, because of destructor which holds reonnection code
        WARN ("unable to connect to %s:%d : %s", b->host, b->port, strerror (errno));
    }
    struct client * client = ichat_s2s_client_create (remote, OUT_AUTH, b->server->config->s2s_me->host, b);
    if (!client)
    {
        WARN ("unable to create s2s client[fd=%d] : %s", remote, strerror (errno));
        close (remote);
        return 0;
    }
    server_add_client (b->server, client);
    return 0;
}

static void
task_start_ichat_s2s_link (void * data)
{
    const struct s2s_block * link_block = (const struct s2s_block *)data;
    start_ichat_s2s_link (link_block);
}

struct timed_task *
make_s2s_link_task (long long delay, const struct s2s_block * b)
{
    return task_create (delay, task_start_ichat_s2s_link, NULL, (void *)b);
}
