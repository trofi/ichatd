#ifndef __CONFIG_H__
#define __CONFIG_H__

// some constants

// wtf?
#define MAX_WORD_LEN       128

#define MAX_READ_BLOCK     512
#define CONF_IOV_MAX       128

#define DEF_USER_PORT      6666
#define DEF_CONTROL_PORT   7777
#define DEF_S2S_PORT       6767

#define DEF_CONFIG_FILE         "./ichatd.conf"
#define DEF_LOG_FILE            "./ichatd.log"
#define DEF_PID_FILE            "./ichatd.pid"
#define DEF_SERVER_NAME         "slyfox's ichatd"
#define DEF_SERVER_PASSWORD     "hello world"

// values in seconds
#define S2S_RECONNECT_DELTA      30
#define S2S_HEARTBEAT_DELTA      (5*60)

// \r\n for telnet compat
#define DEF_CTL_BANNER                                          \
    "Hi!\r\n"                                                   \
    "You've just connected to slyfox's ichatd ctl port!\r\n"    \
    "Go type here something\r\n"

struct server;

struct s2s_block {
    struct s2s_block * next;
    char * host;
    int port;
    char * pass;
    struct server * server;
};

struct config
{
    int    foreground_mode;  // to daemonise or not
    int    user_port;        // port for user's connections
    int    ctl_port;         // port to handle server via telnet

    char * log_file;         // file to store logs to

    int    log_level;        // TODO: some desc, enum levels
    char * pid_file;         // lock file stores PID, tracks other instances
    int    null_clients;     // whether to allow clients to receive data without authentication
    int    max_msg_size;     // max message length in bytes
                             // (messages more than 7KB makes win ichat clents sick)
    struct s2s_block * s2s_queue; // queue of servers to connect to (our server is initiator)
    struct s2s_block * s2s_me; // server description
};

// parse errors
enum CONFIG_STATUS {
    CONFIG_OK = 0,
    // here to place error states
    CONFIG_ENOMEM,
    CONFIG_FORMAT_ERROR,
    CONFIG_FILE_NOT_FOUND,
};

struct config * config_alloc (void);
void config_destroy (struct config * config);

enum CONFIG_STATUS
parse_config (struct config * config, const char * fname);

#endif // __CONFIG_H__
