#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "ichat/client/ichat_client.h"
#include "ichat_server.h"

#include "client.h"
#include "server.h"

#include "list.h"

#include "log.h"

static void ichat_server_read_op(struct server * server, struct client * client);
static void ichat_server_error_op(struct server * server, struct client * client);
static int  ichat_server_can_read_op(struct server * server, struct client * client);

struct ops ichat_server_ops = {
    .read  = ichat_server_read_op,
    .write = NULL,
    .error = ichat_server_error_op,

    .can_read  = ichat_server_can_read_op,
    .can_write = NULL,
    .destroy   = ichat_server_destroy
};

static void
ichat_server_read_op(struct server * server,
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
    struct client * new_client = ichat_client_create (new_client_fd);
    if (!client)
    {
        WARN ("unable to create client. memory runout");
        close (new_client_fd);
        return;
    }
    DEBUG("adding new client to list");
    server_add_client (server, new_client);

    struct sockaddr_in sa;
    socklen_t l = sizeof (struct sockaddr_in);
    getpeername (new_client_fd, (struct sockaddr *)&sa, &l);

    NOTE("ichat client[fd=%d] connected (%s)", new_client_fd, inet_ntoa (sa.sin_addr));
}

static void
ichat_server_error_op(struct server * server,
                      struct client * client)
{
    assert (server);
    assert (client);
    DEBUG (__func__);

    client->corrupt = 1;
}

static int
ichat_server_can_read_op(struct server * server,
                         struct client * client)
{
    assert (server);
    assert (client);
    //TODO: impl some limits (per ip/per second), gather some stats(glo(server) tbl?)
    return 1;
}
