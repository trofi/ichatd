#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "config.h"

#include "client.h"
#include "ctl_client.h"
#include "ctl_client_impl.h"
#include "ctl_client_ops.h"

#include "log.h"

struct client *
ctl_client_create (int fd)
{
    struct client * client = client_create (fd, CTL_CLIENT, ctl_client_ops);
    // FIXME: handle memleaks
    struct ctl_client_impl * impl = ctl_client_create_impl();
    client->impl = impl;
    // FIXME: handle memleaks

    // initial banner
    buffer_set_size(impl->bo, strlen (DEF_CTL_BANNER));
    memcpy (buffer_data(impl->bo), DEF_CTL_BANNER, buffer_size(impl->bo));

    DEBUG ("ctl_client %p created [fd=%d]", client, fd);
    return client;
}

void
ctl_client_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ctl_client %p closed", client);
    if (client->impl)
        ctl_client_destroy_impl(client->impl);
}
