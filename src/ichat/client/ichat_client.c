#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffer.h"
#include "config.h"

#include "client.h"
#include "ichat_client.h"
#include "ichat_client_impl.h"
#include "ichat_client_ops.h"

#include "log.h"

struct client *
ichat_client_create (int fd)
{
    struct client * client = client_create (fd, ICHAT_CLIENT, ichat_client_ops);
    // FIXME: handle memleaks, fdleaks
    struct ichat_client_impl * impl = ichat_client_create_impl();
    client->impl = impl;
    // FIXME: handle memleaks, fdleaks

    DEBUG ("ichat_client created [fd=%d]", fd);
    return client;
}

void
ichat_client_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ichat_client closed[fd=%d]", client->fd);
    struct ichat_client_impl * impl = (struct ichat_client_impl *)(client->impl);
    if (impl)
        ichat_client_destroy_impl(impl);
    // TODO: broadcast DISCONNECT ichat message
}
