#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "misc.h"

// broadcast signatures are: "*", ""
// pretty easy :]

int
ichat_sig_is_broadcast (const struct buffer * sig)
{
    assert (sig);
    const size_t len = buffer_size (sig);
    const char * data = buffer_data (sig);

    if (len == 0 || // "" case
        (len == 1  && * data == '*'))
        return 1;
    return 0;
}

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
