#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "icmisc.h"

// broadcast signatures are: "*", ""
// pretty easy :]

int
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

// strcmp/memcmp alike
int
ichat_sig_cmp (const struct buffer * sig1,
               const struct buffer * sig2)
{
    assert (sig1);
    assert (sig2);

    const size_t len1 = buffer_size (sig1);
    const size_t len2 = buffer_size (sig2);

    if (len1 > len2)
        return 1;

    if (len1 < len2)
        return -1;

    return memcmp (buffer_data (sig1),
                   buffer_data (sig2),
                   len1);
}


//////////
#include "list.h"
#include "log.h"
#include "server.h"
#include "client.h"
#include "ichat/s2s_client/ichat_s2s_client_impl.h"

void ichat_broadcast (struct server * server,
                      struct client * client, // originator
                      struct buffer * msg)
{
    struct client * clnt = 0;
    list_for_each(clnt, server->clist)
    {
        if (clnt->type == ICHAT_CLIENT)
        {
            DEBUG ("[B:local] sending %p -> %p", client, clnt);
            clnt->op.add_message (server, clnt, msg);
        }
        if (clnt->type == ICHAT_S2S_CLIENT
            && clnt != client
            && ((struct ichat_s2s_client_impl *)(clnt->impl))->is_authenticated)
        {
            DEBUG ("[B:s2s] sending %p -> %p", client, clnt);
            clnt->op.add_message (server, clnt, msg);
        }        
    }
}

void
ichat_unicast (struct server * server,
               struct client * client, // originator
               const struct buffer * receiver,
               struct buffer * msg)
{
    assert (server);
    assert (client);
    assert (receiver);
    assert (msg);

    // TODO: unicast on remote server howto? (keep list of clients?)
    struct client * clnt = 0;
    list_for_each(clnt, server->clist)
    {
        if (clnt->type == ICHAT_S2S_CLIENT
            && ichat_sig_cmp (receiver,
                              ((struct ichat_s2s_client_impl *)(clnt->impl))->sig) == 0)
        {
	    DEBUG ("[U:local] sending %p -> %p", client, clnt);
            clnt->op.add_message (server, clnt, msg);
            return; // successfully delivered
        }
    }

    // client isn't local, so send to all servers
    // TODO: keep list of remotely announced clients
    //       to avoud this broadcast
    list_for_each(clnt, server->clist)
    {
        if (clnt->type == ICHAT_S2S_CLIENT
            && clnt != client
            && ((struct ichat_s2s_client_impl *)(clnt->impl))->is_authenticated)
        {
            DEBUG ("[U:s2s] sending %p -> %p", client, clnt);
            clnt->op.add_message (server, clnt, msg);
        }        
    }
}
