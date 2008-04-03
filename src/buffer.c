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
    free (b->data);
    free (b);
}

static void
buffer_enlarge (struct buffer * b, size_t new_size)
{
    assert (b);
    assert (b->rc > 0);
    b->data = (char *)realloc (b->data, new_size);
}

struct buffer *
buffer_alloc ()
{
    struct buffer * b = (struct buffer *)malloc (sizeof (struct buffer));
    //memset (b, 0, sizeof (struct buffer));
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
