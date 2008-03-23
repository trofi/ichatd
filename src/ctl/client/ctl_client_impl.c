#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "buffer_queue.h"

#include "ctl_client_impl.h"

struct ctl_client_impl *
ctl_client_create_impl ()
{
    struct ctl_client_impl * impl = (struct ctl_client_impl *)malloc (sizeof (struct ctl_client_impl));
    if (!impl)
        goto e_no_mem;
    memset (impl, 0, sizeof (struct ctl_client_impl));

    impl->bi = buffer_alloc();
    if (!impl->bi)
        goto e_no_mem;
    
    impl->bo = buffer_queue_alloc();
    if (!impl->bo)
        goto e_no_mem;

    return impl;

  e_no_mem:
    ctl_client_destroy_impl (impl);
    return 0;
}

void
ctl_client_destroy_impl (struct ctl_client_impl * impl)
{
    if (!impl)
        return;
    if (impl->bi)
        buffer_unref (impl->bi);
    if (impl->bo)
        buffer_queue_free (impl->bo);
    free (impl);
}
