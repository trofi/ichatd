#include <string.h>
#include <stdio.h>

#include "../src/buffer.h"

int
main (void)
{
    struct buffer * b = buffer_alloc();
    if (!b)
        return 1;
    if (buffer_size(b) != 0)
        return 2;
    buffer_reserve (b, 1024);
    if (buffer_size(b) != 0)
        return 3;
    buffer_set_size (b, 2048);

    buffer_data(b)[0] = 'A';
    buffer_data(b)[2047] = 'Z';

    if (buffer_size(b) != 2048)
        return 3;
    buffer_unref(b);
    return 0;
}
