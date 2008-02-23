#include <stdlib.h>
#include <string.h>

#include "buffer.h"

#include "ichat_client_impl.h"

//#include "log.h"

struct ichat_client_impl *
ichat_client_create_impl ()
{
    struct ichat_client_impl * impl = (struct ichat_client_impl *)malloc (sizeof (struct ichat_client_impl));
    if (!impl)
        goto e_no_mem;
    memset (impl, 0, sizeof (struct ichat_client_impl));

    impl->bi = buffer_alloc();
    if (!impl->bi)
        goto e_no_mem;
    
    impl->bo = buffer_alloc();
    if (!impl->bo)
        goto e_no_mem;

    impl->sig = buffer_alloc();
    if (!impl->sig)
        goto e_no_mem;

    impl->bytes_written = 0;
    return impl;

  e_no_mem:
    ichat_client_destroy_impl (impl);
    return 0;
}

void
ichat_client_destroy_impl (struct ichat_client_impl * impl)
{
    if (!impl)
        return;
    if (impl->bi)
        buffer_unref(impl->bi);
    if (impl->bo)
        buffer_unref(impl->bo);
    if (impl->sig)
        buffer_unref(impl->sig);
}
