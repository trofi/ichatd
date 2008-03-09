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
    assert (fd >= 0);
    struct client * client = client_create (fd, ICHAT_CLIENT, ichat_client_ops);
    // FIXME: handle memleaks, fdleaks
    struct ichat_client_impl * impl = ichat_client_create_impl();
    client->impl = impl;
    // FIXME: handle memleaks, fdleaks

    DEBUG ("ichat_client %p created [fd=%d]", client, fd);
    return client;
}

void
ichat_client_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ichat_client %p closed", client);
    if (client->impl)
        ichat_client_destroy_impl(client->impl);
    // TODO: broadcast DISCONNECT ichat message
}
