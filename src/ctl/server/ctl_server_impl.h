#ifndef __CTL_SERVER_IMPL_H__
#define __CTL_SERVER_IMPL_H__

struct ctl_server_impl;
struct ctl_server_impl * ctl_server_impl_create (void);
void ctl_server_impl_destroy (struct ctl_server_impl * client);


struct ctl_server_impl
{
    void * reserved;
};

#endif // __CTL_SERVER_IMPL_H__
