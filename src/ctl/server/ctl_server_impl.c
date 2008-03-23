#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "ctl_server.h"
#include "ctl_server_ops.h"
#include "ctl_server_impl.h"

static struct ctl_server_impl *
ctl_server_impl_alloc (void)
{
    struct ctl_server_impl * impl = (struct ctl_server_impl *)malloc (sizeof (struct ctl_server_impl));
    if (!impl)
        goto e_no_mem;
    memset (impl, 0, sizeof (struct ctl_server_impl));

    return impl;

  e_no_mem:
    ctl_server_impl_destroy (impl);
    return 0;
}

struct ctl_server_impl *
ctl_server_impl_create ()
{
    struct ctl_server_impl * impl = ctl_server_impl_alloc();
    if (!impl)
        return 0;
    return impl;
}

void
ctl_server_impl_destroy (struct ctl_server_impl * impl)
{
    if (!impl)
        return;
    free (impl);
}
