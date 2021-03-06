#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "ctl/client/ctl_client.h"

#include "ctl_server.h"

#include "client.h"
#include "server.h"

#include "list.h"

#include "log.h"

static void ctl_server_read_op(struct server * server, struct client * client);
static void ctl_server_error_op(struct server * server, struct client * client);
static int  ctl_server_can_read_op(struct server * server, struct client * client);

struct ops ctl_server_ops = {
    .read  = ctl_server_read_op,
    .write = NULL,
    .error = ctl_server_error_op,

    .can_read  = ctl_server_can_read_op,
    .can_write = NULL,
    .destroy   = ctl_server_destroy
};

static void
ctl_server_read_op(struct server * server,
                   struct client * client)
{
    assert (server);
    assert (client);
    DEBUG("trying to accept new client");
    int new_client_fd = accept (client->fd, NULL, NULL);
    // TODO: check fd validity
    if (new_client_fd == -1)
    {
        // MT: use strerror_r
        WARN ("unable to accept client: %s", strerror (errno));
        return;
    }
    DEBUG("new client = %d", new_client_fd);
    struct client * new_client = ctl_client_create (new_client_fd);
    if (!client)
    {
        WARN ("unable to create client. memory runout");
        close (new_client_fd);
        return;
    }
    DEBUG("adding new client to list");
    server_add_client(server, new_client);
    NOTE("client connected");
}

static void
ctl_server_error_op(struct server * server,
                    struct client * client)
{
    assert (server);
    assert (client);
    DEBUG (__func__);
    client->corrupt = 1;
}

static int
ctl_server_can_read_op(struct server * server,
                       struct client * client)
{
    assert (server);
    assert (client);
    //TODO: impl some limits (per ip/per second), gather some stats(glo(server) tbl?)
    return 1;
}
