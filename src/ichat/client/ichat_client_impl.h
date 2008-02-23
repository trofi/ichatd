#ifndef __ICHAT_CLIENT_IMPL_H__
#define __ICHAT_CLIENT_IMPL_H__

#include <unistd.h> // for size_t

struct buffer;
struct ichat_client_impl;

struct ichat_client_impl * ichat_client_create_impl ();
void ichat_client_destroy_impl (struct ichat_client_impl * impl);

struct ichat_client_impl
{
    struct buffer * bi; // strings form client

    size_t bytes_written;
    struct buffer * bo; // strings to   client

    struct buffer * sig; // client's signature
};

#endif // __ICHAT_CLIENT_IMPL_H__
