#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffer.h"
#include "buffer_queue.h"
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
    // FIXME: !!!handle e_no_mem!!!
    struct ctl_client_impl * impl = ctl_client_create_impl();
    client->impl = impl;
    struct buffer_queue * q = impl->bo;
    assert (q);

    // initial banner
    struct buffer * b = buffer_alloc ();
    {
        size_t b_size = strlen (DEF_CTL_BANNER);
        buffer_set_size(b, b_size);
        memcpy (buffer_data (b), DEF_CTL_BANNER, b_size);
        buffer_queue_append (q, b);
    }
    buffer_unref (b);
    DEBUG ("ctl_client created [fd=%d]", fd);
    return client;
}

void
ctl_client_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ctl_client closed [fd=%d]", client->fd);
    if (client->impl)
        ctl_client_destroy_impl(client->impl);
}
