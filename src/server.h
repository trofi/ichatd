#ifndef __SERVER_H__
#define __SERVER_H__

struct server;
struct client;
struct timed_task;

// allocation/value integrity
struct server * server_alloc (void);
void server_destroy(struct server * server);

// All these funs return:
// 0        - on success
// non-null - in error
enum SERVER_STATUS {
    SERVER_OK = 0,
    // here follows error status
    SERVER_STOP_REQUESTED,
    SERVER_ENOMEM,
    SERVER_BAD_CONFIG,
    SERVER_BAD_CMDARG,
};

enum SERVER_STATUS server_configure (struct server * server, int argc, char * argv[]);
enum SERVER_STATUS server_run (struct server * server);
enum SERVER_STATUS server_shutdown (struct server * server);
enum SERVER_STATUS server_status(struct server * server);

enum SERVER_STATUS server_add_client(struct server * server, struct client * client);
struct client * server_remove_client(struct server * server, struct client * client);

enum SERVER_STATUS server_add_task(struct server * server, struct timed_task * task);
struct timed_task * server_pop_task(struct server * server);

const char * server_error (struct server * server);

struct server
{
    char * config_filename;  // current config file name
    struct config * config;
    struct client * clist;   // list of clients
    struct timed_task * task_queue;
    int    shutdown;          // bool var. whether server is in shutdown state
};

#endif // __SERVER_H__
