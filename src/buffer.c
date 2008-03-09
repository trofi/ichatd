#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>

#include "buffer.h"
#include "config.h"

#include "log.h"

struct buffer
{
    struct buffer * next;
    size_t rc;// ref count
    size_t len;
    size_t capacity;
    char * data;
};

// mm
static void
buffer_free (struct buffer * b)
{
    assert (b);
    free (b);
}

static void
buffer_enlarge (struct buffer * b, size_t new_size)
{
    assert (b);
    assert (b->rc > 0);
    b->data = realloc (b->data, new_size);
}

struct buffer *
buffer_alloc ()
{
    struct buffer * b = (struct buffer *)malloc (sizeof (struct buffer));
    //memset (b, 0, sizeof (struct buffer));
    b->next = 0;
    b->rc = 1; //one ref
    b->capacity = b->len = 0;
    b->data = 0;
    return b;
}

struct buffer *
buffer_ref(struct buffer * b)
{
    assert (b);
    assert (b->rc > 0);
    b->rc++;
    return b;
}

void
buffer_unref(struct buffer * b)
{
    assert (b);
    assert (b->rc > 0);
    if (!--b->rc)
    {
        buffer_free (b); 
    }
}

struct buffer *
buffer_clone (struct buffer * b);

// ops (stl-'like' :])
size_t
buffer_size(const struct buffer * b)
{
    assert(b);
    assert (b->rc > 0);
    return b->len;
}

void
buffer_set_size(struct buffer * b, size_t new_size)
{
    assert (b);
    assert (b->rc > 0);
    if (b->len < new_size)
    {
        size_t old_len = b->len;
        buffer_reserve (b, new_size);
        memset (b->data + old_len, 0, new_size - old_len);
    }
    b->len = new_size;
}

char *
buffer_data(const struct buffer * b)
{
    assert(b);
    assert (b->rc > 0);
    return b->data;
}

struct buffer *
buffer_next(struct buffer * b)
{
    assert (b);
    assert (b->rc > 0);
    return b->next;
}

void
buffer_set_next(struct buffer * b, struct buffer * next_b)
{
    assert (b);
    assert (b->rc > 0);
    assert (next_b);
    assert (next_b->rc > 0);
    // avoid passing buffer into many lists
    assert (b->next == 0);
    //assert (next_b->next == 0);

    b->next = next_b;
}

// allocates AT LEAST new_size bytes
void
buffer_reserve(struct buffer * b, size_t new_size)
{
    assert (b);
    assert (b->rc > 0);
    if (b->capacity >= new_size)
        return;
    buffer_enlarge (b, new_size);
}

ssize_t
buffer_read (struct buffer * b, int fd, size_t size)
{
    assert (b);
    assert (b->rc > 0);
    buffer_reserve (b, b->len + size);
    ssize_t r;
    {
      try_again:
        r = read (fd, b->data + b->len, size);
        if (r == -1)
            return -1;
        if (r == 0 // make it signal protected
            && errno == EINTR)
            goto try_again;
        b->len += r;
    }
    return r;
}

ssize_t
buffer_write (struct buffer * b, size_t off,  int fd, size_t size)
{
    assert (b);
    assert (b->rc > 0);
    assert (b->len >= off + size);
    ssize_t r;
    {
      try_again:
        r = write (fd, b->data + off, size);
        /*
         * // implemented below
         * if (r == -1)
         * return -1;
         */
        if (r == 0 // make it signal protected
            && errno == EINTR)
            goto try_again;
    }
    return r;
    
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
get_buffer_list_iovec (struct buffer * b_list,
                       struct iovec * iov,
                       int * iov_size)
{
    assert (b_list);
    assert (iov);
    assert (iov_size);

    size_t full_avail_size = 0;
    int iov_ops = 0;

    struct buffer * b = b_list;
    for (;
         b && buffer_size (b) && (iov_ops < *iov_size);
         ++iov_ops, b = b->next)
    {
        iov[iov_ops].iov_base = b->data;
        iov[iov_ops].iov_len  = b->len;
        full_avail_size      += b->len;
    }
    *iov_size = iov_ops;
    return full_avail_size;
}

ssize_t
buffer_list_read (struct buffer * b_list, size_t off, int fd)
{
    assert (b_list);

    // WARNING: thread unsafe
    struct iovec iov[CONF_IOV_MAX];
    int iov_cnt = CONF_IOV_MAX;
    get_buffer_list_iovec (b_list, iov, &iov_cnt);
    if (!iov_cnt
        || iov[0].iov_len <= off) //out of buffer
        return -1;

    iov[0].iov_base += off;
    iov[0].iov_len  -= off;
    
    // TODO: fixup first buffer offset
    ssize_t r;
    {
      try_again:
        r = readv (fd, iov, iov_cnt);
        if (r == -1)
            return -1;
        if (r == 0 // make it signal protected
            && errno == EINTR)
            goto try_again;
    }
    return r;
}

ssize_t
buffer_list_write (struct buffer * b_list, size_t off, int fd)
{
    // TODO: implement ;]
    assert (b_list);
    assert (fd >=0);

    // WARNING: thread unsafe
    struct iovec iov[CONF_IOV_MAX];
    int iov_cnt = CONF_IOV_MAX;
    get_buffer_list_iovec (b_list, iov, &iov_cnt);
    if (iov_cnt == 0
        || iov[0].iov_len <= off) //out of buffer
        return -1;

    iov[0].iov_base += off;
    iov[0].iov_len  -= off;

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
    return r;
}
