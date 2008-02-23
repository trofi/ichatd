#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "icc2s.h"

struct icc2s {
    struct buffer * b;

    const char * sender;
    int sender_len;

    const char * command;
    int command_len;

    const char * receiver;
    int receiver_len;

    const char * msg_data;
    int msg_data_len;
};

static void
icc2s_destroy (struct icc2s * c2s_msg);

struct icc2s *
ichat_buffer_to_icc2s (struct buffer * msg)
{
    assert (msg);

    struct icc2s * c2s_msg = (struct icc2s *)malloc (sizeof (struct icc2s));
    if (!c2s_msg)
        goto e_no_mem;
    memset (c2s_msg, 0, sizeof (struct icc2s));

    // TODO: validate inner structure
    struct buffer * b_copy = c2s_msg->b = buffer_ref (msg);
    const char * b_data = buffer_data (b_copy);
    int b_size = buffer_size (b_copy);

    // find sender
    if (b_size <= 0)
        goto e_bad_msg;
    const char * sender_end = memchr (b_data, 0, b_size);
    if (!sender_end)
        goto e_bad_msg;
    c2s_msg->sender = b_data;
    c2s_msg->sender_len = sender_end - b_data;

    b_data = sender_end + 1;
    b_size -= c2s_msg->sender_len + 1; //skip '\0'
    
    //find command
    if (b_size <= 0)
        goto e_bad_msg;
    const char * cmd_end = memchr (b_data, 0, b_size);
    if (!cmd_end)
        goto e_bad_msg;
    c2s_msg->command = b_data;
    c2s_msg->command_len = cmd_end - b_data;

    b_data = cmd_end + 1;
    b_size -= c2s_msg->command_len + 1; //skip '\0'

    //find receiver
    if (b_size <= 0)
        goto e_bad_msg;
    const char * receiver_end = memchr (b_data, 0, b_size);
    if (!receiver_end)
        goto e_bad_msg;
    c2s_msg->receiver = b_data;
    c2s_msg->receiver_len = receiver_end - b_data;

    b_data = receiver_end + 1;
    b_size -= c2s_msg->receiver_len + 1; //skip '\0'

    //store data
    if (b_size < 0)
        goto e_bad_msg;
    c2s_msg->msg_data = b_data;
    c2s_msg->msg_data_len = b_size;
    
    return c2s_msg;

  e_bad_msg:
  e_no_mem:
    icc2s_destroy (c2s_msg);
    return 0;
}

static void
icc2s_destroy (struct icc2s * c2s_msg)
{
    if (!c2s_msg)
        return;
    buffer_unref (c2s_msg->b);
    free (c2s_msg);
}

void
icc2s_unref (struct icc2s * c2s_msg)
{
    icc2s_destroy (c2s_msg);
}

struct buffer *
icc2s_sender (struct icc2s * c2s_msg)
{
    assert (c2s_msg);
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, c2s_msg->sender_len);
    memcpy (buffer_data (b), c2s_msg->sender, c2s_msg->sender_len);
    return b;

  e_no_mem:
    return 0;
}

struct buffer *
icc2s_command (struct icc2s * c2s_msg)
{
    assert (c2s_msg);
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, c2s_msg->command_len);
    memcpy (buffer_data (b), c2s_msg->command, c2s_msg->command_len);
    return b;

  e_no_mem:
    return 0;
}

struct buffer *
icc2s_receiver (struct icc2s * c2s_msg)
{
    assert (c2s_msg);
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, c2s_msg->receiver_len);
    memcpy (buffer_data (b), c2s_msg->receiver, c2s_msg->receiver_len);
    return b;

  e_no_mem:
    return 0;
}

struct buffer *
icc2s_data (struct icc2s * c2s_msg)
{
    assert (c2s_msg);
    
    struct buffer * b = buffer_alloc ();
    if (!b)
        goto e_no_mem;
    buffer_set_size (b, c2s_msg->msg_data_len);
    memcpy (buffer_data (b), c2s_msg->msg_data, c2s_msg->msg_data_len);
    return b;

  e_no_mem:
    return 0;
}
