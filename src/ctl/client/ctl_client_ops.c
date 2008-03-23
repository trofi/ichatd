#include <assert.h>
#include <string.h>
#include <errno.h>

#include "ctl_client.h"
#include "ctl_client_impl.h"
#include "ctl_client_ops.h"

#include "buffer.h"
#include "buffer_queue.h"
#include "client.h"
#include "server.h"
#include "config.h"

#include "list.h"
#include "log.h"
#include "misc.h"

static void ctl_client_read_op(struct server * server, struct client * client);
static void ctl_client_write_op(struct server * server, struct client * client);
static void ctl_client_error_op(struct server * server, struct client * client);
static void ctl_client_add_message (struct server * server, struct client * client, struct buffer * msg);
static int ctl_client_can_read_op(struct server * server, struct client * client);
static int ctl_client_can_write_op(struct server * server, struct client * client);

struct ops ctl_client_ops = {
    .read         = ctl_client_read_op,
    .write        = ctl_client_write_op,
    .error        = ctl_client_error_op,
    .add_message  = ctl_client_add_message,
    .can_read     = ctl_client_can_read_op,
    .can_write    = ctl_client_can_write_op,
    .destroy      = ctl_client_destroy
};

static void
ctl_client_process_message(struct server * server,
                           struct client * client,
                           struct buffer * msg)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    assert (msg);

    DEBUG("client[fd=%d] processes data: [LEN = %zu]", client->fd, buffer_size (msg));
    log_print_array (DEBUG_LEVEL, buffer_data (msg), buffer_size (msg));

    const char * data = buffer_data (msg);
    const size_t d_size = buffer_size (msg);

    struct buffer * response = buffer_alloc ();

#define IS_KW(keyword) strncmp (data, keyword, min (d_size, strlen (keyword))) == 0
    if (IS_KW("echo"))
    {
        buffer_unref (response);
        response = buffer_ref (msg);
    }
    else if (IS_KW("help"))
    {
        static const char resp[] =
            "Command list i understand:\r\n"
            "help - this message\r\n"
            "echo <some text> - i'll respond you the same text\r\n"
            "exit - close connection\r\n"
            ;
        static const size_t resp_len = sizeof (resp) - 1;

        buffer_set_size (response, resp_len);
        memcpy (buffer_data (response), resp, resp_len);        
    }
    else if (IS_KW("exit"))
    {
        client->corrupt = 1;
    }
    else
    {
        static const char resp[] =
            "unknown command\r\n"
            "try help to get more info\r\n";
        static const size_t resp_len = sizeof (resp) - 1;

        buffer_set_size (response, resp_len);
        memcpy (buffer_data (response), resp, resp_len);
    }
#undef IS_KW

    // currently we simply add this data at end of response to this client
    ctl_client_add_message (server, client, response);

    buffer_unref (response);
}

static void
ctl_client_read_op(struct server * server,
                   struct client * client)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    struct ctl_client_impl * impl = client->impl;
    assert (impl);
    ssize_t result = buffer_read (impl->bi,
                                  client->fd,
                                  MAX_READ_BLOCK);
    switch (result)
    {
        case -1:
            NOTE ("ctl client[fd=%d] read error: %s", client->fd, strerror (errno));
        case  0:
            client->corrupt = 1;
            return;
            break;
        default:
            break;
    }

    // TODO: check for input buffer overflow
    const char * p = buffer_data(impl->bi);
    size_t s = buffer_size(impl->bi);
    const char * end = p + s;
    while ((end = (char *)memchr (p, '\n', s)))
    {
        ++end; //advance to next to '\n'
        struct buffer * b = buffer_alloc();
        buffer_set_size(b, end - p);
        memcpy (buffer_data (b), p, end - p);
        ctl_client_process_message (server, client, b);
        buffer_unref (b);
        s -= end - p;
    }
    if (s)
    {
        memmove (buffer_data (impl->bi), p, s);
        buffer_set_size(impl->bi, s);
    }
    else
        buffer_set_size (impl->bi, 0);
}

static void
ctl_client_write_op(struct server * server,
                    struct client * client)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    struct ctl_client_impl * impl = client->impl;
    assert (impl);

    if (buffer_queue_size (impl->bo) == 0)
        return;

    ssize_t result = buffer_queue_write (impl->bo, client->fd);
    switch (result)
    {
        case -1:
            NOTE ("ctl client[fd=%d] write error: %s", client->fd, strerror (errno));
        case  0:
            client->corrupt = 1;
            return;
            break;
        default:
            break;
    }
}

static void
ctl_client_error_op(struct server * server,
                    struct client * client)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    client->corrupt = 1;
}

static void
ctl_client_add_message (struct server * server,
                        struct client * client,
                        struct buffer * msg)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    assert (msg);

    struct ctl_client_impl * impl = client->impl;
    assert (impl);

    struct buffer_queue * q = impl->bo;
    buffer_queue_append (q, msg);
}

static int
ctl_client_can_read_op(struct server * server,
                       struct client * client)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    return !client->corrupt;
}

static int
ctl_client_can_write_op(struct server * server,
                        struct client * client)
{
    DEBUG (__func__);
    assert (server);
    assert (client);

    // TODO: check for avail mq and msgio buffer
    struct ctl_client_impl * impl = client->impl;
    assert (impl);
    assert (impl->bo);

    if (client->corrupt)
        return 0;
    return buffer_queue_size (impl->bo);
}
