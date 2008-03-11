#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "ics2s.h"
#include "icmisc.h" // make_timestamp

struct ics2s {
    struct buffer * b;

    const char * sender;
    int sender_len;

    const char * timestamp;
    int timestamp_len;

    const char * command;
    int command_len;
    
    const char * msg_data;
    int msg_data_len;
};

static void
ics2s_destroy (struct ics2s * s2s_msg);

struct ics2s *
ichat_buffer_to_ics2s (struct buffer * msg)
{
    assert (msg);

    struct ics2s * s2s_msg = (struct ics2s *)malloc (sizeof (struct ics2s));
    if (!s2s_msg)
        goto e_no_mem;
    memset (s2s_msg, 0, sizeof (struct ics2s));

    // TODO: validate inner structure
    struct buffer * b_copy = s2s_msg->b = buffer_ref (msg);
    const char * b_data = buffer_data (b_copy);
    int b_size = buffer_size (b_copy);

    // find sender
    if (b_size <= 0)
        goto e_bad_msg;
    const char * sender_end = memchr (b_data, 0, b_size);
    if (!sender_end)
        goto e_bad_msg;
    s2s_msg->sender = b_data;
    s2s_msg->sender_len = sender_end - b_data;

    b_data = sender_end + 1;
    b_size -= s2s_msg->sender_len + 1; //skip '\0'

    //find timestamp
    if (b_size <= 0)
        goto e_bad_msg;
    const char * timestamp_end = memchr (b_data, 0, b_size);
    if (!timestamp_end)
        goto e_bad_msg;
    s2s_msg->timestamp = b_data;
    s2s_msg->timestamp_len = timestamp_end - b_data;

    b_data = timestamp_end + 1;
    b_size -= s2s_msg->timestamp_len + 1; //skip '\0'

    //find command
    if (b_size <= 0)
        goto e_bad_msg;
    const char * cmd_end = memchr (b_data, 0, b_size);
    if (!cmd_end)
        goto e_bad_msg;
    s2s_msg->command = b_data;
    s2s_msg->command_len = cmd_end - b_data;

    b_data = cmd_end + 1;
    b_size -= s2s_msg->command_len + 1; //skip '\0'

    //store data
    if (b_size < 0)
        goto e_bad_msg;
    s2s_msg->msg_data = b_data;
    s2s_msg->msg_data_len = b_size;
    
    return s2s_msg;

  e_bad_msg:
  e_no_mem:
    ics2s_destroy (s2s_msg);
    return 0;
}

static void
ics2s_destroy (struct ics2s * s2s_msg)
{
    if (!s2s_msg)
        return;
    buffer_unref (s2s_msg->b);
    free (s2s_msg);
}

void
ics2s_unref (struct ics2s * s2s_msg)
{
    ics2s_destroy (s2s_msg);
}

struct buffer *
ics2s_sender (struct ics2s * s2s_msg)
{
    assert (s2s_msg);
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, s2s_msg->sender_len);
    memcpy (buffer_data (b), s2s_msg->sender, s2s_msg->sender_len);
    return b;

  e_no_mem:
    return 0;
}

struct buffer *
ics2s_timestamp (struct ics2s * s2s_msg)
{
    assert (s2s_msg);
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, s2s_msg->timestamp_len);
    memcpy (buffer_data (b), s2s_msg->timestamp, s2s_msg->timestamp_len);
    return b;

  e_no_mem:
    return 0;
}

struct buffer *
ics2s_command (struct ics2s * s2s_msg)
{
    assert (s2s_msg);
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, s2s_msg->command_len);
    memcpy (buffer_data (b), s2s_msg->command, s2s_msg->command_len);
    return b;

  e_no_mem:
    return 0;
}

struct buffer *
ics2s_data (struct ics2s * s2s_msg)
{
    assert (s2s_msg);
    
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, s2s_msg->msg_data_len);
    memcpy (buffer_data (b), s2s_msg->msg_data, s2s_msg->msg_data_len);
    return b;

  e_no_mem:
    return 0;
}


////////
#include "config.h"
#include "misc.h"

struct buffer *
s2s_make_login_msg (const char * server_name, const char * password)
{
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;

    const char * command = "LOGIN";
    // FIXME: fill it with normal data
    char timestamp[18];
    make_timestamp (timestamp);

    // [len][0x00][sender][0x00][timestamp][0x00][cmd][0x00][pass]
    size_t len = strlen (server_name) + 1 + strlen (timestamp) + 1 + strlen (command) + 1 + strlen (password);
    size_t full_len = number_len (len) + 1 + len;
    buffer_set_size (b, full_len + 1); // +1 of '\0'
    snprintf (buffer_data (b), full_len + 1,
              "%zu%c%s%c%s%c%s%c%s",
                len, '\0',
                    server_name, '\0',
                        timestamp, '\0',
                            command, '\0',
                                password);

    buffer_set_size (b, full_len); // trim last '\0'
    return b;

  e_no_mem:
    return 0;
}
