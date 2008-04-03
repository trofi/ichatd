#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ichat_s2s_server.h"
#include "ichat_s2s_server_ops.h"
#include "ichat_s2s_server_impl.h"

#include "log.h"

struct client *
ichat_s2s_server_create (int fd)
{
    struct client * client = client_create (fd, ICHAT_S2S_SERVER, ichat_s2s_server_ops);
    //FIXME: handle memleaks
    client->impl = ichat_s2s_server_impl_create ();
    //FIXME: handle memleaks
    DEBUG ("ichat_s2s_server[fd=%d] created", client->fd);
    return client;
}

void
ichat_s2s_server_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ichat_s2s_server[fd=%d] closed", client->fd);
    struct ichat_s2s_server_impl * impl = (struct ichat_s2s_server_impl *)(client->impl);
    if (impl)
        ichat_s2s_server_impl_destroy (impl);
}
