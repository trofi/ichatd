#include <stdlib.h>
#include <string.h>

#include "buffer.h"

#include "ichat_s2s_client_impl.h"

//#include "log.h"

struct ichat_s2s_client_impl *
ichat_s2s_client_create_impl (enum AUTH_DIR auth_dir)
{
    struct ichat_s2s_client_impl * impl = (struct ichat_s2s_client_impl *)malloc (sizeof (struct ichat_s2s_client_impl));
    if (!impl)
        goto e_no_mem;
    memset (impl, 0, sizeof (struct ichat_s2s_client_impl));

    impl->bi = buffer_alloc();
    if (!impl->bi)
        goto e_no_mem;
    
    impl->bo = buffer_alloc();
    if (!impl->bo)
        goto e_no_mem;

    impl->sig = buffer_alloc();
    if (!impl->sig)
        goto e_no_mem;

    impl->auth_dir = auth_dir;
    impl->is_authenticated = 0;
    impl->bytes_written = 0;
    
    return impl;

  e_no_mem:
    ichat_s2s_client_destroy_impl (impl);
    return 0;
}

void
ichat_s2s_client_destroy_impl (struct ichat_s2s_client_impl * impl)
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
