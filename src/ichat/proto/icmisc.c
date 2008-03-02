#include <assert.h>
#include <string.h>

#include "buffer.h"

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
