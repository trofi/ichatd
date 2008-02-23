#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "ichat_server.h"
#include "ichat_server_ops.h"
#include "ichat_server_impl.h"

static struct ichat_server_impl *
ichat_server_impl_alloc (void)
{
    struct ichat_server_impl * priv = (struct ichat_server_impl *)malloc (sizeof (struct ichat_server_impl));
    if (!priv)
        goto e_no_mem;
    memset (priv, 0, sizeof (struct ichat_server_impl));

    return priv;

  e_no_mem:
    ichat_server_impl_destroy (priv);
    return 0;
}

struct ichat_server_impl *
ichat_server_impl_create ()
{
    struct ichat_server_impl * priv = ichat_server_impl_alloc();
    if (!priv)
        return 0;
    return priv;
}

void
ichat_server_impl_destroy (struct ichat_server_impl * priv)
{
    if (!priv)
        return;
    free (priv);
}
