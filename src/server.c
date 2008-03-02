//
// All the file is full of mess
// It's more experimenting field, than structural
// Most of content should be moved out
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"


#include "client.h"
#include "server.h"
#include "ichat/server/ichat_server.h"
#include "ctl/server/ctl_server.h"

#include "poller.h"
#include "log.h"
#include "misc.h"
#include "list.h"
#include "log.h"
#include "task.h"

struct server *
server_alloc (void)
{
    struct server * server = (struct server *)malloc (sizeof (struct server));
    if (!server)
        goto e_no_mem;
    memset (server, 0, sizeof (struct server));
    
    if (!(server->config = config_alloc ()))
        goto e_no_mem;
    if (!(server->config_filename = strdup (DEF_CONFIG_FILE)))
        goto e_no_mem;
    return server;

  e_no_mem:
    server_destroy (server);
    return 0;
}

void
server_destroy (struct server * server)
{
    if (!server) return;

    free (server->config_filename);
    config_destroy (server->config);
    free (server);
}

enum SERVER_STATUS
server_configure (struct server * server, int argc, char * argv[])
{
    assert (server);

    struct config * config = server->config;
    int c;

    while (1)
    {
        static struct option long_options[] = {
            { "foreground", 0, 0, 'f' },
            { "pid-file",   1, 0, 'p' },
            { "config",     1, 0, 'c' },
            { "help",       0, 0, 'h' },
            { 0,            0, 0,  0  }
        };

        c = getopt_long(argc, argv, "fp:c:h", long_options, NULL);
        if (c == -1)
            break;

        switch (c)
        {
            case 'f':
                config->foreground_mode = 1;
                break;
            case 'p':
                free (config->pid_file);
                if (!(config->pid_file = strdup (optarg)))
                    return SERVER_ENOMEM;
                break;
            case 'c':
                free (server->config_filename);
                if (!(server->config_filename = strdup (optarg)))
                    return SERVER_ENOMEM;
                break;
            case 'h':
            {
                unsigned int i;
                for (i = 0; i < sizeof long_options / sizeof long_options[0] - 1; ++i)
                    fprintf(stderr, "  --%s%s\n", long_options[i].name, long_options[i].has_arg ? "=<arg>":"");
                server_destroy (server);
                exit (EXIT_SUCCESS);
            }
            break;
            default:
                //TODO: format error string
                fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
                return SERVER_BAD_CMDARG;
        }
    }

    if (optind < argc)
    {
        fprintf (stderr, "non-option ARGV-elements: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
        return SERVER_BAD_CMDARG;
    }
    if (parse_config (config, server->config_filename))
        return SERVER_BAD_CONFIG; //TODO: fill correctly error string
    return SERVER_OK;
}

enum SERVER_STATUS
server_add_client(struct server * server, struct client * client)
{
    list_prepend (client, server->clist);
    return SERVER_OK;
}

struct client *
server_remove_client(struct server * server, struct client * client)
{
    // be carefull here.
    // I'm iterating through memory places
    // where pointers are stored (not only their values)

    struct client ** phead = &server->clist;
    for (;*phead; phead = &((*phead)->next))
    {
        if (*phead == client)
        {
            DEBUG("dequeued client");
            *phead = (*phead)->next;
            return client;
        }
    }
    return 0;
}

enum SERVER_STATUS
server_add_task (struct server * server, struct timed_task * task)
{
    if (!server->task ||
        task->time < server->task->time)
    {
        task->next = server->task;
        server->task = task;
        return SERVER_OK;
    }

    struct timed_task * prev = server->task;
    // find place to drop in task
    while (prev->next
           && prev->next->time < task->time)
        prev = prev->next;

    // insert right after prev
    task->next = prev->next;
    prev->next = task->next;
    return SERVER_OK;
}

struct timed_task *
server_pop_task (struct server * server)
{
    if (!server->task)
        return SERVER_OK;
    struct timed_task * prev = server->task;

    server->task = prev->next;
    return prev;
}

const char *
server_error (struct server * server)
{
    if (!server)
        return "unable to allocate memory";
    return "All ok";
}

static int server_init_log_subsystem (struct server * server);
static int server_detach (struct server * server);
static int server_bind_ports (struct server * server);
static int server_register_heartbeats (struct server * server);
static int server_register_s2s_links (struct server * server);
static enum SERVER_STATUS server_start_dispatcher (struct server * server);

enum SERVER_STATUS
server_run (struct server * server)
{

    assert (server);
    //FIXME: handle errors
    server_init_log_subsystem (server);
    server_detach (server);
    server_bind_ports (server);
    server_register_heartbeats (server);
    server_register_s2s_links (server);
    server_start_dispatcher (server);

    return SERVER_OK;
}


static int server_cleanup_dispatcher (struct server * server);
static int server_close_ports (struct server * server);
static int server_close_log_subsystem (struct server * server);

enum SERVER_STATUS
server_shutdown (struct server * server)
{
    assert (server);

    server_cleanup_dispatcher (server);
    server_close_ports (server);
    server_close_log_subsystem (server);
    
    //FIXME: handle errors, proper shutdown
    return SERVER_OK;
}

/////////////////////////////////////
// here goes private implementation
////////////////////////////////////
static int
server_init_log_subsystem (struct server * server)
{
    struct config * config = server->config;
    open_log (config->log_file);
    set_log_level (config->log_level);
    DEBUG("log subsystem inited");
    return 0;
}

static int
server_close_log_subsystem (struct server * server UNUSED_SYMBOL)
{
    DEBUG("closing log subsystem");
    close_log ();
    return 0;
}

static int
server_bind_ports (struct server * server)
{
    struct config * config = server->config;

    if (config->user_port)
    {
        int cli_server_fd = IC_bind_server_socket ("0.0.0.0", config->user_port);
        DEBUG("binding server socket for ichat clients");
        if (cli_server_fd > 0)
        {    
            struct client * cli_server = (struct client *)ichat_server_create (cli_server_fd);
            if (!cli_server)
            {
                WARN ("unable to bind ichat server socket for clients");
                close (cli_server_fd);
            }
            else
            {
                list_prepend(cli_server, server->clist);
            }
        }
        else
        {
            WARN ("unable to bind server ichat socket for clients");
        }
    }
    
    if (config->ctl_port)
    {
        int ctl_server_fd = IC_bind_server_socket ("127.0.0.1", config->ctl_port);
        DEBUG("binding server socket for ctl connections");
        if (ctl_server_fd > 0)
        {
            struct client * ctl_server = (struct client *)ctl_server_create (ctl_server_fd);
            if (!ctl_server)
            {
                WARN ("unable to bind ichat control socket");
                close (ctl_server_fd);
            }
            else
            {
                list_prepend(ctl_server, server->clist);
            }
        }
        else
        {
            WARN ("unable to bind ichat control socket");
        }
    }
    return 0;
}

static void
heartbeat (void * data)
{
    struct server ** server = data;
    NOTE ("-- HEARTBEAT --");
    // reregister event
    server_register_heartbeats (*server);
}

static int
server_register_heartbeats (struct server * server)
{
    // hack for free() in dtor of tack
    // TODO: rework destructor interface

    struct server ** data = malloc (sizeof(struct server *));
    *data = server;
    // 5 min
    struct timed_task * task = task_create (1000 * 5 * 60, heartbeat, data);
    server_add_task (server, task);
    return 0;
}

////////////////

#include "ichat/s2s_client/ichat_s2s_client.h"

static int
start_s2s_link (struct server * server, const char * host, int port, const char * password)
{
    int remote = IC_nonblock_connect (host, port);
    struct client * client = ichat_s2s_client_create (remote, OUT_AUTH, password);
    server_add_client (server, client);
    return 0;
}

static int
server_register_s2s_links (struct server * server)
{
    // TODO: FIXME: here can be server set to connect to
    // start_s2s_link (server, host, port, pass);
    return 0;
}

////////////////////

static int
server_close_ports (struct server * server)
{
    assert (server);
    return 0;
}

static int
server_detach (struct server * server)
{
    assert (server);
    if (!server->config->foreground_mode)
        return 0;
    if (getppid () != 1) // our parent is not init
    {    
        int forker;
        struct sigaction sa;
        // ignore some signals...
        // TODO - why we do it here ? 
        // why not after fork ?
        sa.sa_handler = SIG_IGN;
        sigaction (SIGTTIN, &sa, NULL);
        sigaction (SIGTTOU, &sa, NULL);
        sigaction (SIGTSTP, &sa, NULL);
    
        // fork returns 0 in the child thread or -1 upon error
        // otherwise it will return PID of new process
        forker = fork ();
        if (forker == -1)
            return 1;
        else if (forker != 0) // i.e. we are in the caller
            return 0;

        // set us as a process group leader... make anything else... stupid call. why we can't use CreateProcess
        // with well documented API ?       
        setsid ();
    }
    /*
    // i'd like todo it cleaner
    struct sigaction sa;
    memset (&sa, 0,sizeof(sa)); 
    
    sa.sa_handler = sig_hup_hndlr;
    sigaction (SIGHUP, &sa, NULL);

    sa.sa_handler = sig_term_hndlr;
    sigaction (SIGTERM, &sa, NULL);

    sa.sa_handler = sig_segv_hndlr;
    sigaction (SIGSEGV, &sa, NULL);
    
    sa.sa_handler = SIG_IGN;
    sigaction (SIGPIPE, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGALRM, &sa, NULL);
    */
    return 0;
}

static enum SERVER_STATUS
server_start_dispatcher (struct server * server)
{
    assert (server);
    for (;;)
    {
        enum POLL_RESULT result = server_poll (server);
        if (result == POLL_ERROR)
            return SERVER_OK; //FIXME: set proper error
    }
    return SERVER_OK;
}

static int
server_cleanup_dispatcher (struct server * server)
{
    assert (server);
    return 0;
}
