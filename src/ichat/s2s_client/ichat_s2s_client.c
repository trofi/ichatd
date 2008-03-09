#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "config.h"

#include "client.h"
#include "ichat_s2s_client.h"
#include "ichat_s2s_client_impl.h"
#include "ichat_s2s_client_ops.h"

#include "log.h"

struct client *
ichat_s2s_client_create (int fd, enum AUTH_DIR auth_dir, const char * my_name, const char * password)
{
    assert (fd >= 0);
    struct client * client = client_create (fd, ICHAT_S2S_CLIENT, ichat_s2s_client_ops);
    // FIXME: handle memleaks, fdleaks
    struct ichat_s2s_client_impl * impl = ichat_s2s_client_create_impl (auth_dir, my_name, password);
    client->impl = impl;
    // FIXME: handle memleaks, fdleaks

    DEBUG ("ichat_s2s_client created [fd=%d]", fd);
    return client;
}

void
ichat_s2s_client_destroy (struct client * client)
{
    if (!client) return;
    DEBUG ("ichat_s2s_client closed [fd=%d]", client->fd);
    if (client->impl)
        ichat_s2s_client_destroy_impl(client->impl);
}
