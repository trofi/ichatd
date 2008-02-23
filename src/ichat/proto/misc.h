#ifndef __ICHAT_PROTO_MISC_H__
#define __ICHAT_PROTO_MISC_H__

// some limith
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

// returns:
//   1 - if addr is broadcast
//   0 - else

struct buffer;
int ichat_sig_is_broadcast (const struct buffer * sig);

// strcmp/memcmp alike
int ichat_sig_cmp (const struct buffer * sig1, const struct buffer * sig2);

#endif // __ICHAT_PROTO_MISC_H__
