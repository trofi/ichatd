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

// returns:
//   1 - if addr is broadcast
//   0 - else

struct buffer;
int ichat_sig_is_broadcast (const struct buffer * sig);

// strcmp/memcmp alike
int ichat_sig_cmp (const struct buffer * sig1, const struct buffer * sig2);

// dispatch stuff
struct server;
struct client;
// TODO: place them to separate header
void ichat_broadcast (struct server * server,
                      struct client * client, // originator
                      struct buffer * msg);
void
ichat_unicast (struct server * server,
               struct client * client, // originator
               const struct buffer * receiver,
               struct buffer * msg);

#endif // __ICMISC_H__
