#ifndef __ICHAT_CLIENT_H__
#define __ICHAT_CLIENT_H__

struct client * ichat_client_create (int fd);
void ichat_client_destroy (struct client * client);

#endif // __ICHAT_CLIENT_H__
