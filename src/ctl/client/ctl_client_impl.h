#ifndef __CTL_CLIENT_IMPL_H__
#define __CTL_CLIENT_IMPL_H__

#include <unistd.h> // for size_t

struct buffer;
struct ctl_client_impl;

struct ctl_client_impl * ctl_client_create_impl ();
void ctl_client_destroy_impl (struct ctl_client_impl * impl);

struct ctl_client_impl
{
    struct buffer * bi; // strings form client

    size_t bytes_written;
    struct buffer * bo; // strings to   client
};

#endif // __CTL_CLIENT_IMPL_H__
