#ifndef __CTL_SERVER_H__
#define __CTL_SERVER_H__

#include "client.h"

// TODO: how about ABI, opacity mix?

struct ctl_server
{
    CLIENT;
};

struct client * ctl_server_create (int fd);
void ctl_server_destroy (struct client * client);

#endif // __CTL_SERVER_H__
