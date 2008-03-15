#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ichat_server.h"
#include "ichat_server_ops.h"
#include "ichat_server_impl.h"

#include "log.h"

struct client *
ichat_server_create (int fd)
{
    struct client * client = client_create (fd, ICHAT_SERVER, ichat_server_ops);
    //FIXME: handle memleaks
    client->impl = ichat_server_impl_create ();
    //FIXME: handle memleaks
    DEBUG ("ichat_server created [fd=%d]", fd);
    return client;
}

void
ichat_server_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ichat_server closed [fd=%d]", client->fd);
    if (client->impl)
        ichat_server_impl_destroy (client->impl);
}
