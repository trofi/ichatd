#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "buffer.h"

#include "misc.h" // GetTimerMS
#include "icmisc.h"

// strcmp/memcmp alike
int
ichat_sig_cmp (const struct buffer * sig1,
               const struct buffer * sig2)
{
    assert (sig1);
    assert (sig2);

    const size_t len1 = buffer_size (sig1);
    const size_t len2 = buffer_size (sig2);

    if (len1 > len2)
        return 1;

    if (len1 < len2)
        return -1;

    return memcmp (buffer_data (sig1),
                   buffer_data (sig2),
                   len1);
}

const char *
make_timestamp (char * in_buff)
{
    assert (in_buff);
    time_t t;
    struct tm *tmp;
    t = time (NULL);
    // WARN: thread unsafe
    tmp = localtime(&t);
    size_t bytes = strftime (in_buff, 18, "%Y%m%d%H%M%S", tmp);
    // TODO: check for success
    snprintf (in_buff + bytes, 4, "%03u", (int)(GetTimerMS() % 1000));
    return in_buff;
}
