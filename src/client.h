#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "ops.h"

//macro for inheritance
#define CLIENT            \
    struct client * next; \
    int fd;               \
    int corrupt;          \
    int type;             \
    struct ops op;        \
    void * impl;

struct client
{
    CLIENT;
};
enum CLIENT_TYPE {
    CTL_SERVER,
    CTL_CLIENT,
    ICHAT_SERVER,
    ICHAT_CLIENT,
    ICHAT_S2S_SERVER,
    ICHAT_S2S_CLIENT,
};
struct client * client_create (int fd, int type, struct ops op);
void client_destroy (struct client * client);

#endif // __CLIENT_H__
