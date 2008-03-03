#ifndef __ICHAT_S2S_CLIENT_H__
#define __ICHAT_S2S_CLIENT_H__

enum AUTH_DIR {
    IN_AUTH,  // this client accepts authentication data
    OUT_AUTH, // this client receives authentication data
};
struct client * ichat_s2s_client_create (int fd, enum AUTH_DIR auth_dir, const char * password);
void ichat_s2s_client_destroy (struct client * client);

#endif // __ICHAT_S2S_CLIENT_H__