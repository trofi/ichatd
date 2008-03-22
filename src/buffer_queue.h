#ifndef __BUFFER_QUEUE_H__
#define __BUFFER_QUEUE_H__

#include <unistd.h>

struct buffer;
struct buffer_queue;

struct buffer_queue * buffer_queue_alloc ();
void buffer_queue_free (struct buffer_queue * q);

size_t buffer_queue_size (struct buffer_queue * q);

struct buffer_queue * buffer_queue_append (struct buffer_queue * q, struct buffer * b);
struct buffer_queue * buffer_queue_prepend (struct buffer_queue * q, struct buffer * b);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IO API: should go out of here
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// (readv return value semantics)
// -1 - error, 0 - eof
ssize_t buffer_queue_write (struct buffer_queue * q, int fd);

#endif // __BUFFER_QUEUE_H__
