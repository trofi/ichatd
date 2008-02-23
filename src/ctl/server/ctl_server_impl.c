#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "ctl_server.h"
#include "ctl_server_ops.h"
#include "ctl_server_impl.h"

static struct ctl_server_impl *
ctl_server_impl_alloc (void)
{
    struct ctl_server_impl * priv = (struct ctl_server_impl *)malloc (sizeof (struct ctl_server_impl));
    if (!priv)
        goto e_no_mem;
    memset (priv, 0, sizeof (struct ctl_server_impl));

    return priv;

  e_no_mem:
    ctl_server_impl_destroy (priv);
    return 0;
}

struct ctl_server_impl *
ctl_server_impl_create ()
{
    struct ctl_server_impl * priv = ctl_server_impl_alloc();
    if (!priv)
        return 0;
    return priv;
}

void
ctl_server_impl_destroy (struct ctl_server_impl * priv)
{
    if (!priv)
        return;
    free (priv);
}
