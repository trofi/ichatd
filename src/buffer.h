#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <unistd.h>

struct buffer;

// mm
struct buffer * buffer_alloc ();
struct buffer * buffer_ref(struct buffer * b);
//size_t buffer_ref_count(struct buffer * b);
void buffer_unref(struct buffer * b);
struct buffer * buffer_clone (struct buffer * b);

// ops (stl-like)

size_t buffer_size(const struct buffer * b);
//may trim, zero expands
void buffer_set_size(struct buffer * b, size_t new_size);

char * buffer_data(const struct buffer * b);

//allocates AT LEAST new_size bytes
void buffer_reserve(struct buffer * b, size_t new_size);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IO API: should go out of here
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// enlarges and reads, returns number of fetched characters
// -1 - error, 0 - eof
ssize_t buffer_read (struct buffer * b, int fd, size_t size);
ssize_t buffer_write (struct buffer * b, size_t off,  int fd, size_t size);

#endif // __BUFFER_H__
