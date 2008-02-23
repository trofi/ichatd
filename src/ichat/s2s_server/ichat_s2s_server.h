#ifndef __ICHAT_S2S_SERVER_H__
#define __ICHAT_S2S_SERVER_H__

#include "client.h"

// TODO: how about ABI, opacity mix?

struct ichat_s2s_server
{
    CLIENT;
};

struct client * ichat_s2s_server_create (int fd);
void ichat_s2s_server_destroy (struct client * client);

#endif // __ICHAT_S2S_SERVER_H__
