#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"

#include "log.h"

static struct client *
client_alloc (void)
{
    struct client * client = (struct client *)malloc (sizeof (struct client));
    if (!client)
        goto e_no_mem;
    memset (client, 0, sizeof (struct client));
    return client;

  e_no_mem:
    client_destroy (client);
    return 0;
}

struct client *
client_create (int fd, int type, struct ops op)
{
    struct client * client = client_alloc();
    if (!client)
        return 0;
    client->op = op;
    client->fd = fd;
    client->type = type;
    client->corrupt = 0;
    DEBUG ("client %p created[fd=%d]", client, client->fd);
    return client;
}

void
client_destroy (struct client * client)
{
    if (!client)
        return;
    DEBUG ("client %p closed[fd=%d]", client, client->fd);
    
    close (client->fd);
    if (client->op.destroy)
        client->op.destroy (client);
    free (client);
}
