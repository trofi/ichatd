#ifndef __ICHAT_DISPATCH_H__
#define __ICHAT_DISPATCH_H__

struct server;
struct client;
struct buffer;

void ichat_dispatch (struct server * server,
                     struct client * client, // originator
                     struct buffer * msg); // isc2s format

#endif // __ICHAT_DISPATCH_H__
