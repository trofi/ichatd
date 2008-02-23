#ifndef __ICHAT_S2S_CLIENT_IMPL_H__
#define __ICHAT_S2S_CLIENT_IMPL_H__

#include <unistd.h> // for size_t

#include "ichat_s2s_client.h"  // for AUTH_TYPE

struct buffer;
struct ichat_s2s_client_impl;

struct ichat_s2s_client_impl * ichat_s2s_client_create_impl (enum AUTH_DIR auth_dir);
void ichat_s2s_client_destroy_impl (struct ichat_s2s_client_impl * impl);

struct ichat_s2s_client_impl
{
    struct buffer * bi; // strings form client

    size_t bytes_written;
    struct buffer * bo; // strings to   client

    struct buffer * sig; // client's signature

    enum AUTH_DIR auth_dir;
    int is_authenticated; // whether to interop with this client
};

#endif // __ICHAT_S2S_CLIENT_IMPL_H__
