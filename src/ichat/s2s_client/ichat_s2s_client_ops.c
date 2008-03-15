#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

#include "ichat_s2s_client.h"
#include "ichat_s2s_client_impl.h"
#include "ichat_s2s_client_ops.h"

#include "buffer.h"
#include "client.h"
#include "server.h"
#include "config.h"

#include "list.h"
#include "log.h"
#include "misc.h"

#include "ichat/proto/ics2s.h"
#include "ichat/proto/icmisc.h"
#include "ichat/proto/ichat_dispatch.h"

static void ichat_s2s_client_read_op(struct server * server, struct client * client);
static void ichat_s2s_client_write_op(struct server * server, struct client * client);
static void ichat_s2s_client_error_op(struct server * server, struct client * client);

static void ichat_s2s_client_add_message(struct server * server, struct client * client, struct buffer * msg);

static int ichat_s2s_client_can_read_op(struct server * server, struct client * client);
static int ichat_s2s_client_can_write_op(struct server * server, struct client * client);

struct ops ichat_s2s_client_ops = {
    .read         = ichat_s2s_client_read_op,
    .write        = ichat_s2s_client_write_op,
    .error        = ichat_s2s_client_error_op,
    .add_message  = ichat_s2s_client_add_message,
    .can_read     = ichat_s2s_client_can_read_op,
    .can_write    = ichat_s2s_client_can_write_op,
    .destroy      = ichat_s2s_client_destroy
};

static void
ichat_s2s_client_process_message (struct server * server,
                                  struct client * client,
                                  struct buffer * msg)
{
    assert (server);
    assert (client);
    assert (msg);
    struct ichat_s2s_client_impl * impl = client->impl;
    assert (impl);

    struct ics2s * icmsg = ichat_buffer_to_ics2s (msg);
    if (!icmsg)
    {
        NOTE ("%s: s2s client[fd=%d] sent malformed message", __func__, client->fd);
        log_print_array (DEBUG_LEVEL, buffer_data (msg), buffer_size (msg));
        client->corrupt = 1;
        return;
    }

    struct buffer * sig = ics2s_sender (icmsg);
    // FIXME: servers don't substitute server names on packet retransmit
    // so we should not write it to log + we should not mangle server
    // names and timestamps.
    if (ichat_sig_cmp (sig, impl->sig))
    {
        buffer_unref (impl->sig);
        impl->sig = sig;
        DEBUG ("%s: s2s client[fd=%d] registered with sig:", __func__, client->fd);
        log_print_array (DEBUG_LEVEL, buffer_data (sig), buffer_size (sig));
    }
    else
    {
        buffer_unref (sig);
    }

    struct buffer * clnt_msg = ics2s_data (icmsg);
    struct buffer * cmd = ics2s_command (icmsg);

#define CMD_IS(command) (buffer_size(cmd) == strlen(command)            \
                         && memcmp (buffer_data(cmd), command, strlen(command)) == 0)
    if (CMD_IS("LOGIN"))
    {
        // TODO: make a delay/cpu load not to bruteforce passes

        // here clnt msg means sent password
        const char * real_pass = server->config->s2s_me->pass;
        size_t real_pass_len = strlen (real_pass);
        if (buffer_size (clnt_msg) == real_pass_len
            && memcmp (buffer_data (clnt_msg), real_pass, real_pass_len) == 0)
        {
            impl->is_authenticated = 1;
            NOTE ("s2s client[fd=%d] authenticated. Welcome", client->fd);
        }
        else
        {
            WARN ("s2s client[fd=%d] sent wrong password", client->fd);
            log_print_array (DEBUG_LEVEL, buffer_data (clnt_msg), buffer_size (clnt_msg));
        }
    }
    else if (CMD_IS("FORWARD"))
    {
        if (impl->is_authenticated)
        {
            // stripping out message size
            size_t buf_sz = buffer_size (clnt_msg);
            char * buf_data = buffer_data (clnt_msg);

            char * zero_pos = memchr (buf_data, '\0', buf_sz);
            if (zero_pos)
            {
                struct buffer * b = buffer_alloc ();
                size_t new_size = buf_sz - (zero_pos + 1 - buf_data);

                buffer_set_size (b, buf_sz);
                memcpy (buffer_data (b), zero_pos + 1, new_size);

                ichat_dispatch (server, client, b);

                buffer_unref (b);
            }
            else
            {
                NOTE ("%s: s2s client[fd=%d] sent malformed FORWARD message. drop it", __func__, client->fd);
                log_print_array (DEBUG_LEVEL, buffer_data (clnt_msg), buffer_size (clnt_msg));
            }
        }
        else
        {
            NOTE ("%s: s2s client[fd=%d] tries to send data without being authenticated", __func__, client->fd);
        }
    }
    else
    {
        // TODO: dump exact command
        WARN ("%s: s2s client[fd=%d] sent strange command:", __func__, client->fd);
        log_print_array (WARN_LEVEL, buffer_data (cmd), buffer_size (cmd));
    }
#undef CMD_IS
    buffer_unref (cmd);
    buffer_unref (clnt_msg);
    ics2s_unref (icmsg);
}

static void
ichat_s2s_client_read_op(struct server * server,
                     struct client * client)
{
    assert (server);
    assert (client);
    struct ichat_s2s_client_impl * impl = client->impl;
    assert (impl);
    ssize_t result = buffer_read (impl->bi,
                                  client->fd,
                                  MAX_READ_BLOCK);
    switch (result)
    {
        case -1:
            DEBUG ("%s: ichat s2s client[fd=%d] read error (%s)", __func__, client->fd, strerror (errno));
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
            NOTE ("%s: s2s client[fd=%d] header doesn't have header length", __func__, client->fd);
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
            NOTE ("%s: s2s client[fd=%d] sent bad packet: len = %s", __func__, client->fd, p);
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
            DEBUG ("got message len = %d from s2s client[fd=%d]", msg_size, client->fd);
            ichat_s2s_client_process_message (server, client, b);
            buffer_unref (b);
        }

        size_t rest_data_size = s - expected_buffer_size;

        if (rest_data_size)
            memmove (buffer_data (impl->bi), p + expected_buffer_size, rest_data_size);
        buffer_set_size (impl->bi, rest_data_size);
    }
}

static void
ichat_s2s_client_write_op(struct server * server,
                          struct client * client)
{
    DEBUG (__func__);
    assert (server);
    assert (client);
    struct ichat_s2s_client_impl * impl = client->impl;
    assert (impl);
    assert (impl->bo);

    // if buffer - write buffer part
    // dispatcher, pipe manager
    if (buffer_size (impl->bo) == 0)
        return;

    ssize_t result = buffer_list_write (impl->bo,
                                        impl->bytes_written,
                                        client->fd);
    switch (result)
    {
        case -1:
            DEBUG ("%s: ichat s2s client[fd=%d] write error (%s)", __func__, client->fd, strerror (errno));
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
ichat_s2s_client_error_op(struct server * server,
                      struct client * client)
{
    assert (server);
    assert (client);
    client->corrupt = 1;
}

// TODO: add arbitrary data injection to data channels
// client/server2server testing purpose
// [msg] -> [server][timestamp][command][msg]
static void
ichat_s2s_client_add_message (struct server * server,
                              struct client * client,
                              struct buffer * msg)
{
    assert (server);
    assert (client);
    assert (msg);

    size_t msg_size = buffer_size (msg);
    assert (msg_size > MIN_ICHAT_MESSAGE_LEN && msg_size < MAX_ICHAT_MESSAGE_LEN);

///////////////////////////
    // cmd - is [server][timestamp][command]
    // FIXME: we must not substitute remote server's name with our
    // TODO: change chat message represettation to s2s (to keep orig server's name)
    const char * my_name = server->config->s2s_me->host;
    const char * command = "FORWARD";

    size_t cmd_size = strlen (my_name) + 1 + 17 /* timestamp*/ + 1 + strlen (command) + 1 + number_len(msg_size) + 1;
    struct buffer * cmd    = buffer_alloc ();
    buffer_set_size (cmd, cmd_size);

    char timestamp[18];

    make_timestamp (timestamp);
    snprintf (buffer_data (cmd), cmd_size,
              "%s%c%s%c%s%c%zu", my_name, '\0', timestamp, '\0', command, '\0', msg_size);
    buffer_set_size (cmd, cmd_size);
    buffer_set_next (cmd, buffer_ref (msg));
        
///////////////////////////

    struct buffer * msg_head = buffer_alloc ();
    size_t new_msg_size = buffer_size (cmd) + buffer_size (msg);

    size_t msg_head_size = number_len (new_msg_size) + 1; // + '\0'
    buffer_set_size (msg_head, msg_head_size);
    sprintf (buffer_data (msg_head), "%zu", new_msg_size);
 
    buffer_set_next (msg_head, cmd);

    struct ichat_s2s_client_impl * impl = client->impl;
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
ichat_s2s_client_can_read_op(struct server * server,
                         struct client * client)
{
    assert (server);
    assert (client);
    return !client->corrupt;
}

static int
ichat_s2s_client_can_write_op(struct server * server,
                          struct client * client)
{
    assert (server);
    assert (client);

    // TODO: check for avail mq and msgio buffer
    struct ichat_s2s_client_impl * impl = client->impl;
    assert (impl);
    assert (impl->bo);

    if (client->corrupt)
        return 0;
    return buffer_size (impl->bo);
}
