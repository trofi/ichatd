#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>

#include "config.h"

#include "buffer_queue.h"
#include "buffer.h"

//====--------

struct queue_entry
{
    struct queue_entry * next;
    struct buffer * data;
};

static struct queue_entry *
queue_entry_alloc (struct buffer * b)
{
    assert(b);
    struct queue_entry * e = (struct queue_entry *)malloc (sizeof (struct queue_entry));
    if (!e)
        goto e_no_mem;

    e->data = buffer_ref (b);
    return e;

  e_no_mem:
    return 0;
}

static void
queue_entry_free (struct queue_entry * e)
{
    if (!e) return;
    buffer_unref (e->data);
    free (e);
}

//====--------

struct buffer_queue
{
    struct queue_entry * head;
    struct queue_entry * last;
    size_t offset; // holds fist buffer offset (partially written case)
};

void
buffer_queue_free (struct buffer_queue * q)
{
    assert (q);
    struct queue_entry * e = q->head;
    while (e)
    {
        struct queue_entry * prev = e;
        e = e->next;
        queue_entry_free (prev);
    }
    free (q);
}

struct buffer_queue *
buffer_queue_alloc()
{
    struct buffer_queue * q = (struct buffer_queue *)malloc (sizeof (struct buffer_queue));
    q->head = q->last = 0;
    q->offset = 0;
    return q;
}

size_t
buffer_queue_size (struct buffer_queue * q)
{
    assert (q);
    size_t result = 0;
    struct queue_entry * e = q->head;
    while (e)
    {
        result += buffer_size (e->data);
        e = e->next;
    }
    result -= q->offset;
    return result;
}

struct buffer_queue *
buffer_queue_append (struct buffer_queue * q, struct buffer * b)
{
    assert (q);
    assert (b);

    struct queue_entry * e = queue_entry_alloc (b);
    if (!e) goto e_no_mem;

    e->next = 0;

    if (!q->head || !q->last)
    {
        q->head = q->last = e;
    }
    else
    {
        q->last->next = e;
        q->last = e;
    }
    return q;

  e_no_mem:
    return 0;
}

struct buffer_queue *
buffer_queue_prepend (struct buffer_queue * q, struct buffer * b)
{
    assert (q);
    assert (b);

    struct queue_entry * e = queue_entry_alloc (b);
    if (!e) goto e_no_mem;

    e->next = q->head;

    if (!q->head || !q->last)
    {
        q->head = q->last = e;
    }
    else
    {
        q->head = e;
    }
    
    return q;

  e_no_mem:
    return 0;
}

static struct buffer *
buffer_queue_peek_front (struct buffer_queue * q)
{
    assert (q);
    if (!q->head)
        return 0;

    return q->head->data;
}

static struct buffer *
buffer_queue_pop_front (struct buffer_queue * q)
{
    assert (q);
    if (!q->head) // empty queue
        return 0;

    struct queue_entry * e = q->head;
    if (q->head == q->last) // one element in queue
    {
        q->head = q->last = 0;
    }
    else
    {
        q->head = e->next;
    }

    struct buffer * b = buffer_ref (e->data);
    queue_entry_free (e);
    return b;
}

static void
buffer_queue_advance_offset (struct buffer_queue * q, size_t delta)
{
    assert (q);

    delta += q->offset; // i'm too lazy to treat it as special case

    while (delta)
    {
        struct buffer * b = buffer_queue_peek_front (q);
        assert (b);

        size_t b_size = buffer_size (b);
        if (b_size <= delta)
        {
            delta -= b_size;
            buffer_unref (buffer_queue_pop_front (q));
        }
        else
            break;
    }
    q->offset = delta;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//   API optimized for scatter/gather readv/writev ops
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// returns max avail input size
// WARNING: returns actual pointers to data in buffers
//          buffer resize can move data pointer
// returns : data can be read
//           filled iov's (in iov_size)

static size_t
get_buffer_queue_iovec (struct buffer_queue * q,
                        struct iovec * iov,
                        int * iov_size)
{
    assert (q);
    assert (iov);
    assert (iov_size);

    size_t full_avail_size = 0;
    int iov_ops = 0;

    struct queue_entry * e = q->head;
    for (;
         e && (iov_ops < *iov_size);
         ++iov_ops, e = e->next)
    {
        struct buffer * b = e->data;
        iov[iov_ops].iov_base = buffer_data (b);
        iov[iov_ops].iov_len  = buffer_size (b);
        full_avail_size      += buffer_size (b);
    }
    *iov_size = iov_ops;
    return full_avail_size;
}

ssize_t
buffer_queue_write (struct buffer_queue * q, int fd)
{
    assert (q);
    assert (fd >=0);

    struct iovec iov[CONF_IOV_MAX];
    int iov_cnt = CONF_IOV_MAX;
    get_buffer_queue_iovec (q, iov, &iov_cnt);
    if (iov_cnt == 0
        || iov[0].iov_len <= q->offset) //out of buffer
        return -1;

    iov[0].iov_base += q->offset;
    iov[0].iov_len  -= q->offset;

    ssize_t r;
    {
      try_again:
        r = writev (fd, iov, iov_cnt);
        if (r == -1)
            return -1;
        if (r == 0 // make it signal protected
            && errno == EINTR)
            goto try_again;
    }

    buffer_queue_advance_offset (q, r);
    return r;
}
