#ifndef __OPS_H__
#define __OPS_H__

struct server;
struct client;
struct buffer;

typedef void (*CLIENT_OP)(struct server * server,
                          struct client * client);

typedef void (*CLIENT_MSG_OP)(struct server * server,
                              struct client * client,
                              struct buffer * msg);

typedef int  (*CLIENT_OP_BOOL)(struct server * server,
                               struct client * client);
typedef void (*CLIENT_DTOR)(struct client * client);

// Describes operations for clients.
struct ops
{
    CLIENT_OP read;
    CLIENT_OP write;
    CLIENT_OP error;

    CLIENT_MSG_OP add_message;
    
    CLIENT_OP_BOOL can_read;
    CLIENT_OP_BOOL can_write;

    CLIENT_DTOR destroy;
};

#endif // __OPS_H__
