#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "ctl_server.h"
#include "ctl_server_ops.h"
#include "ctl_server_impl.h"

#include "log.h"

struct client *
ctl_server_create (int fd)
{
    struct client * client = client_create (fd, CTL_SERVER, ctl_server_ops);
    //FIXME: handle memleaks
    client->impl = ctl_server_impl_create ();
    //FIXME: handle memleaks
    DEBUG ("ctl_server[fd=%d] created", client->fd);
    return client;
}

void
ctl_server_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ctl_server[fd=%d] closed", client->fd);
    struct ctl_server_impl * impl = (struct ctl_server_impl *)(client->impl);
    if (impl)
        ctl_server_impl_destroy (impl);
}
