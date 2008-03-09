#ifndef __ICMISC_H__
#define __ICMISC_H__

// some limits
enum { 
    // TODO: compute it properly
    MIN_ICHAT_MESSAGE_LEN = 20,
    // TODO: check max client's message
    MAX_ICHAT_MESSAGE_LEN = 4192
};

// This header containg ictat protocol related
// constants and functions. I couldn't find
// proper place for them. When I'll do find -
// this file will be deleted.

//
// takes at least 18 bytes long buffer and fills
// it by string: YYYYMMDDHHmmssuuu\0
//
const char * make_timestamp (char * in_buff);

struct buffer;

// strcmp/memcmp alike
int ichat_sig_cmp (const struct buffer * sig1, const struct buffer * sig2);

#endif // __ICMISC_H__
