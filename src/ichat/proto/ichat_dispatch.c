#include <assert.h>
#include <string.h>

#include "buffer.h"

#include "ichat_dispatch.h"
#include "icmisc.h"
#include "icc2s.h"

// broadcast signatures are: "*", ""
// pretty easy :]

static int
ichat_sig_is_broadcast (const struct buffer * sig)
{
    assert (sig);
    const size_t len = buffer_size (sig);
    const char * data = buffer_data (sig);

    if (len == 0 || // "" case
        (len == 1  && * data == '*'))
        return 1;
    return 0;
}

//////////
#include "list.h"
#include "log.h"
#include "server.h"
#include "client.h"
#include "ichat/s2s_client/ichat_s2s_client_impl.h"

static void
ichat_broadcast (struct server * server,
                 struct client * client, // originator
                 struct buffer * msg)
{
    struct client * clnt = 0;
    list_for_each(clnt, server->clist)
    {
        switch (clnt->type)
        {
            case ICHAT_CLIENT:
                DEBUG ("[B:local] sending [fd=%d] -> [fd=%d]", client->fd, clnt->fd);
                clnt->op.add_message (server, clnt, msg);
                break;
            case ICHAT_S2S_CLIENT:
                if (clnt != client
                    && ((struct ichat_s2s_client_impl *)(clnt->impl))->is_authenticated)
                {
                    DEBUG ("[B:s2s] sending [fd=%d] -> [fd=%d]", client->fd, clnt->fd);
                    clnt->op.add_message (server, clnt, msg);
                }
                break;
            default:
                // skip all the rest
                break;
        }
    }
}

static void
ichat_unicast (struct server * server,
               struct client * client, // originator
               const struct buffer * receiver,
               struct buffer * msg)
{
    assert (server);
    assert (client);
    assert (receiver);
    assert (msg);

    int locally_delivered = 0;
    // TODO: unicast on remote server howto? (keep list of clients?)
    struct client * clnt = 0;
    list_for_each(clnt, server->clist)
    {
        if (clnt->type == ICHAT_CLIENT
            && ichat_sig_cmp (receiver,
                              ((struct ichat_s2s_client_impl *)(clnt->impl))->sig) == 0)
        {
            DEBUG ("[U:local] sending [fd=%d] -> [fd=%d]", client->fd, clnt->fd);
            clnt->op.add_message (server, clnt, msg);

            locally_delivered = 1;
        }
    }

    if (locally_delivered)
        return;
    // client isn't local, so send to all servers
    // TODO: keep list of remotely announced clients
    //       to avoud this broadcast
    list_for_each(clnt, server->clist)
    {
        if (clnt->type == ICHAT_S2S_CLIENT
            && clnt != client
            && ((struct ichat_s2s_client_impl *)(clnt->impl))->is_authenticated)
        {
            DEBUG ("[U:s2s] sending [fd=%d] -> [fd=%d]", client->fd, clnt->fd);
            clnt->op.add_message (server, clnt, msg);
        }        
    }
}

void ichat_dispatch (struct server * server,
                     struct client * client, // originator
                     struct buffer * msg)    // isc2s format
{
    assert (server);
    assert (client);
    assert (msg);

    struct icc2s * c2s_msg = ichat_buffer_to_icc2s (msg);
    if (!c2s_msg)
        return;

    struct buffer * receiver = icc2s_receiver (c2s_msg);
    if (!receiver)
        goto end;

    if (ichat_sig_is_broadcast (receiver))
        ichat_broadcast (server, client, msg);
    else
        ichat_unicast (server, client, receiver, msg);

    buffer_unref (receiver);
  end:
    icc2s_unref (c2s_msg);
}
