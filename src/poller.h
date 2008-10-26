#ifndef __POLLER_H__
#define __POLLER_H__


enum POLL_RESULT {
    POLL_OK = 0,
    POLL_IDLE,
    POLL_ERROR,
};

struct server;
struct client;

enum POLL_RESULT
server_poll (struct server * server);

#endif // __POLLER_H__
