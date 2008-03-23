#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "buffer_queue.h"
#include "config.h"

#include "ichat_s2s_client_impl.h"

#include "ichat/proto/ics2s.h"

struct ichat_s2s_client_impl *
ichat_s2s_client_create_impl (enum AUTH_DIR auth_dir, const char * my_name, const struct s2s_block * b)
{
    struct ichat_s2s_client_impl * impl = (struct ichat_s2s_client_impl *)malloc (sizeof (struct ichat_s2s_client_impl));
    if (!impl)
        goto e_no_mem;
    memset (impl, 0, sizeof (struct ichat_s2s_client_impl));

    impl->bi = buffer_alloc();
    if (!impl->bi)
        goto e_no_mem;
    
    impl->bo = buffer_queue_alloc();
    if (!impl->bo)
        goto e_no_mem;

    impl->sig = buffer_alloc();
    if (!impl->sig)
        goto e_no_mem;

    if (!(impl->my_name = strdup (my_name)))
        goto e_no_mem;

    impl->auth_dir = auth_dir;

    switch (auth_dir)
    {
        case IN_AUTH:
            impl->is_authenticated = 0;
            break;
        case OUT_AUTH:
        {
            // send login message immediately
            // TODO: FIXME: stop these hacks with buffers
            //              implement normal data send interface
            //              check for memleaks
            struct buffer * auth_msg = s2s_make_login_msg (my_name, b->pass);
            {
                buffer_queue_append (impl->bo, auth_msg);
            }
            buffer_unref (auth_msg);
            impl->is_authenticated = 1;
            break;
        }
    }

    impl->link_block = b;

    return impl;

  e_no_mem:
    ichat_s2s_client_destroy_impl (impl);
    return 0;
}

#include "server.h"
#include "ichat_s2s_link.h"

void
ichat_s2s_client_destroy_impl (struct ichat_s2s_client_impl * impl)
{
    if (!impl)
        return;

    // register reconnection task
    if (impl->link_block
        && impl->auth_dir == OUT_AUTH /* we are client */)
    {
        struct server * server = impl->link_block->server;
        if (!server->shutdown)
        {
            struct timed_task * t = make_s2s_link_task (1000 * S2S_RECONNECT_DELTA, impl->link_block);
            server_add_task (impl->link_block->server, t);
        }
    }

    if (impl->bi)
        buffer_unref (impl->bi);
    if (impl->bo)
        buffer_queue_free (impl->bo);
    if (impl->sig)
        buffer_unref (impl->sig);
    free ((char*)impl->my_name);
    free (impl);
}
