#ifndef __ICHAT_SERVER_IMPL_H__
#define __ICHAT_SERVER_IMPL_H__

struct ichat_server_impl;
struct ichat_server_impl * ichat_server_impl_create (void);
void ichat_server_impl_destroy (struct ichat_server_impl * client);


struct ichat_server_impl
{
    void * reserved;
};

#endif // __ICHAT_SERVER_IMPL_H__
