#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

#include "ichat_client.h"
#include "ichat_client_impl.h"
#include "ichat_client_ops.h"

#include "buffer.h"
#include "client.h"
#include "server.h"
#include "config.h"

#include "list.h"
#include "log.h"
#include "misc.h"

#include "ichat/proto/icc2s.h"
#include "ichat/proto/icmisc.h"
#include "ichat/proto/ichat_dispatch.h"

static void ichat_client_read_op(struct server * server, struct client * client);
static void ichat_client_write_op(struct server * server, struct client * client);
static void ichat_client_error_op(struct server * server, struct client * client);

static void ichat_client_add_message(struct server * server, struct client * client, struct buffer * msg);

static int ichat_client_can_read_op(struct server * server, struct client * client);
static int ichat_client_can_write_op(struct server * server, struct client * client);

struct ops ichat_client_ops = {
    .read         = ichat_client_read_op,
    .write        = ichat_client_write_op,
    .error        = ichat_client_error_op,
    .add_message  = ichat_client_add_message,
    .can_read     = ichat_client_can_read_op,
    .can_write    = ichat_client_can_write_op,
    .destroy      = ichat_client_destroy
};

static void
ichat_client_process_message (struct server * server,
                              struct client * client,
                              struct buffer * msg)
{
    assert (server);
    assert (client);
    assert (msg);
    struct ichat_client_impl * impl = client->impl;
    assert (impl);

    // validate
    struct icc2s * icmsg = ichat_buffer_to_icc2s (msg);
    if (!icmsg)
    {
        NOTE ("%s: client[fd=%d] sent malformed message", __func__, client->fd);
        client->corrupt = 1;
        return;
    }

    // gather some info
    struct buffer * sig = icc2s_sender (icmsg);
    if (ichat_sig_cmp (sig, impl->sig))
    {
        buffer_unref (impl->sig);
        impl->sig = sig;
        // TODO: FIXME: prettyprint sig
        NOTE ("%s: client[fd=%d] registered with sig:", __func__, client->fd);
        log_print_array (NOTE_LEVEL, buffer_data (sig), buffer_size (sig));
    }
    else
    {
        buffer_unref (sig);
    }

    ichat_dispatch (server, client, msg);
    icc2s_unref (icmsg);
}

static void
ichat_client_read_op(struct server * server,
                     struct client * client)
{
    assert (server);
    assert (client);
    struct ichat_client_impl * impl = client->impl;
    assert (impl);
    ssize_t result = buffer_read (impl->bi,
                                  client->fd,
                                  MAX_READ_BLOCK);
    switch (result)
    {
        case -1:
            DEBUG ("%s: ichat client[fd=%d] read error (%s)", __func__, client->fd, strerror (errno));
        case  0:
            client->corrupt = 1;
            return;
            break;
        default:
            break;
    }
    const size_t MAX_NUMBER_LEN = 20; // including '\0'
    // TODO: check for input buffer overflow
    
    for (;;)
    {
        size_t s = buffer_size(impl->bi);
        if (s < MAX_NUMBER_LEN)
            return;

        const char * p = buffer_data(impl->bi);

        const char * number_end = memchr (p, '\0', MAX_NUMBER_LEN);
        if (!number_end)
            return;

        char * end_ptr;
        errno = 0;
        long msg_size = strtol (p, &end_ptr, 10);
        if (end_ptr == p)
        {
            NOTE ("%s: client[fd=%d]: header doesn't have header length", __func__, client->fd);
            client->corrupt = 1;
            return;
        }
        if (// number parsing validation
            (errno == ERANGE && (msg_size == LONG_MAX || msg_size == LONG_MIN))
            || (errno != 0 && msg_size == 0)
            // sanity validation
            || msg_size < MIN_ICHAT_MESSAGE_LEN
            || msg_size > MAX_ICHAT_MESSAGE_LEN)
        {
            NOTE ("%s: client[fd=%d] sent bad packet: len = %s", __func__, client->fd, p);
            client->corrupt = 1;
            return;
        }
        const char * msg_data_start = number_end + 1; //skip '\0'
        size_t expected_buffer_size = msg_size + (msg_data_start - p);
        if (expected_buffer_size > s)
            return;

        {
            struct buffer * b = buffer_alloc();
            buffer_set_size(b, msg_size);
            memcpy (buffer_data (b), msg_data_start, msg_size);
            DEBUG ("got message len = %d from client[fd=%d]", msg_size, client->fd);
            ichat_client_process_message (server, client, b);
            buffer_unref (b);
        }

        size_t rest_data_size = s - expected_buffer_size;

        if (rest_data_size)
            memmove (buffer_data (impl->bi), p + expected_buffer_size, rest_data_size);
        buffer_set_size (impl->bi, rest_data_size);
    }
}

static void
ichat_client_write_op(struct server * server,
                      struct client * client)
{
    assert (server);
    assert (client);
    struct ichat_client_impl * impl = client->impl;
    assert (impl);

    // if buffer - write buffer part
    // dispatcher, pipe manager
    if (!impl->bo
        || buffer_size(impl->bo) == 0)
        return;

    ssize_t result = buffer_list_write (impl->bo,
                                        impl->bytes_written,
                                        client->fd);
    switch (result)
    {
        case -1:
            DEBUG ("ctl client[fd=%d] write error: %s", client->fd, strerror (errno));
        case  0:
            client->corrupt = 1;
            return;
            break;
        default:
            break;
    }
    // slight hack ;] (i'm lazy)
    // (no buffer offsets)
    result += impl->bytes_written;
    impl->bytes_written = 0;

    while (result
           && impl->bo
           && (size_t)result >= buffer_size(impl->bo))
    {
        struct buffer * old_head = impl->bo;
        impl->bo = buffer_next (impl->bo);
        result -= buffer_size(old_head);
        buffer_unref (old_head);
    }
    impl->bytes_written = result;
    if (!impl->bo)
        impl->bo = buffer_alloc();
}

static void
ichat_client_error_op(struct server * server,
                      struct client * client)
{
    assert (server);
    assert (client);
    client->corrupt = 1;
}

// TODO: add arbitrary data injection to data channels
// client/server2server testing purpose
// [len][sender][cmd][recver][data] -> [len][cmd][data]
static void
ichat_client_add_message (struct server * server,
                          struct client * client,
                          struct buffer * msg)
{
    assert (server);
    assert (client);
    assert (msg);

    size_t msg_size = buffer_size (msg);
    assert (msg_size > MIN_ICHAT_MESSAGE_LEN && msg_size < MAX_ICHAT_MESSAGE_LEN);

///////////////////////////
    struct icc2s * icmsg = ichat_buffer_to_icc2s (msg);
    assert (icmsg);
    struct buffer * cmd  = icc2s_command (icmsg);
    struct buffer * z    = buffer_alloc (); buffer_set_size (z, 1); buffer_data (z)[0] = '\0'; 
    struct buffer * data = icc2s_data (icmsg);

    buffer_set_next (z, data);
    buffer_set_next (cmd, z);
        
///////////////////////////

    struct buffer * msg_head = buffer_alloc ();
    size_t new_msg_size = buffer_size(cmd) + 1 /* '\0' */ + buffer_size(data);

    size_t msg_head_size = number_len (new_msg_size) + 1; // + '\0'
    buffer_set_size (msg_head, msg_head_size);
    sprintf (buffer_data (msg_head), "%zu", new_msg_size);
 
    buffer_set_next (msg_head, cmd);

    struct ichat_client_impl * impl = client->impl;
    assert (impl);

    struct buffer * b = impl->bo;

    // currently we simply add this data at end of response to this client
    if (buffer_size (b))
    {
        while (buffer_next (b))
            b = buffer_next (b);
        buffer_set_next (b, msg_head);
    }
    else
    {
        buffer_unref(impl->bo);
        impl->bo = msg_head;
    }
}

static int
ichat_client_can_read_op(struct server * server,
                         struct client * client)
{
    assert (server);
    assert (client);
    return !client->corrupt;
}

static int
ichat_client_can_write_op(struct server * server,
                          struct client * client)
{
    assert (server);
    assert (client);

    // TODO: check for avail mq and msgio buffer
    struct ichat_client_impl * impl = client->impl;
    assert (impl);
    assert (impl->bo);

    if (client->corrupt)
        return 0;
    return buffer_size (impl->bo);
}
