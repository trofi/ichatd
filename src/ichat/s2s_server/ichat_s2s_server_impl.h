#ifndef __ICHAT_S2S_SERVER_IMPL_H__
#define __ICHAT_S2S_SERVER_IMPL_H__

struct ichat_s2s_server_impl;
struct ichat_s2s_server_impl * ichat_s2s_server_impl_create (void);
void ichat_s2s_server_impl_destroy (struct ichat_s2s_server_impl * client);


struct ichat_s2s_server_impl
{
    void * reserved;
};

#endif // __ICHAT_S2S_SERVER_IMPL_H__
