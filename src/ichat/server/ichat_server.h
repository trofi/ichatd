#ifndef __ICHAT_SERVER_H__
#define __ICHAT_SERVER_H__

#include "client.h"

// TODO: how about ABI, opacity mix?

struct ichat_server
{
    CLIENT;
};

struct client * ichat_server_create (int fd);
void ichat_server_destroy (struct client * client);

#endif // __ICHAT_SERVER_H__
