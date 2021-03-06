#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "ichat_server.h"
#include "ichat_server_ops.h"
#include "ichat_server_impl.h"

static struct ichat_server_impl *
ichat_server_impl_alloc (void)
{
    struct ichat_server_impl * impl = (struct ichat_server_impl *)malloc (sizeof (struct ichat_server_impl));
    if (!impl)
        goto e_no_mem;
    memset (impl, 0, sizeof (struct ichat_server_impl));

    return impl;

  e_no_mem:
    ichat_server_impl_destroy (impl);
    return 0;
}

struct ichat_server_impl *
ichat_server_impl_create ()
{
    struct ichat_server_impl * impl = ichat_server_impl_alloc();
    if (!impl)
        return 0;
    return impl;
}

void
ichat_server_impl_destroy (struct ichat_server_impl * impl)
{
    if (!impl)
        return;
    free (impl);
}
