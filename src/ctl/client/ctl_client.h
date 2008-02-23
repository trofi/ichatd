#ifndef __CTL_CLIENT_H__
#define __CTL_CLIENT_H__

struct client * ctl_client_create (int fd);
void ctl_client_destroy (struct client * client);

#endif // __CTL_CLIENT_H__
