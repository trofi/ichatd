#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#ifdef LINUX_PPOLL
#define __USE_GNU
#define _GNU_SOURCE
#include <poll.h>
#else
#include <sys/poll.h>
#endif // LINUX_PPOLL

#include "globals.h"
#include "log.h"
#include "listener.h"
#include "clist.h"
#include "misc.h"
#include "options.h"

// TODO: move vars to local context
// LOCALS
// make all vars clean by default
static int usersocket   = -1;
static int serversocket = -1;

static struct sockaddr_in useraddr;
static struct sockaddr_in serveraddr;

//
static void
sig_hup_hndlr (int signo UNUSED_SYMBOL)
{
    IMPORTANT("SIGNAL: HUP");
    g_need2restart = 1;
}

static void
sig_segv_hndlr (int signo UNUSED_SYMBOL)
{
    IMPORTANT("SIGNAL: SEGV");
    g_need2exit = 1;
}

static void
sig_term_hndlr (int signo UNUSED_SYMBOL)
{
    IMPORTANT("SIGNAL: TERM");
    g_need2exit = 1;
}

// some further definitions...
static void * client_thread (void * parameter); // client thread
static int client_accept (int s, int user); // accept new client
static int pollall (void);

// oki. main and its support
void
cleanup (void)
{
    DEBUG("cleaning up");
    // close primary socket...
    if (usersocket != -1)
    {
        DEBUG("users' socket");
        close (usersocket);
        usersocket = -1;
    }
    
    if (serversocket != -1)
    {
        DEBUG("servers' socket");
        close (serversocket);
        serversocket = -1;
    }
    // free memory, allocated for users...
    if (g_userlist)
    {
        DEBUG("users' list");
        clist_clean (g_userlist);
        free (g_userlist);
        g_userlist = NULL;
    }
    
    if (g_serverlist)
    {
        DEBUG("servers' list");
        clist_clean (g_serverlist);
        free (g_serverlist);
        g_serverlist = NULL;
    }
    
    if (g_timelist)
    {
        DEBUG("timelist");
        CLISTITEM * temp = g_timelist->head;
    
        while (temp)
        {
            g_timelist->head = temp->next;
            time_clean ((SERVERTIME*) temp);
        }
        clist_clean (g_timelist);
        free (g_timelist);
        g_timelist = NULL;
    }
    DEBUG("closing log");
    close_log ();
    unlink (g_pidfile);
}

// returns socket handke or -1 on error
int
opensocket (int port, struct sockaddr_in * addr)
{
    int mysocket = -1;
    DEBUG("setting up socket");
    mysocket = socket (AF_INET, SOCK_STREAM, 0);
    if (mysocket == -1)
    {
        FATAL("ERROR: Socket creating error (port: %i): %s", port, strerror (errno));
        return -1;
    }
    // setup socket for reusing address if specified
    // wow ! it works! 
    // oki. after having chatserver shutdowned, thereare pending client connection
    // linger does not help, so they are hanging around about minute or two (dead timeout)
    // but the server could not be g_restarted because socket address is in use...
    // settings SO_REUSEADDR will allow us to open new listening sockets
    // in case of lost pending connections... but it will not allow us to run chat again :)
    if (g_reuseaddr)
    {
        DEBUG("setting setsockopt");
        int opt = 1;
        if (setsockopt (mysocket, SOL_SOCKET, SO_REUSEADDR, & opt, sizeof (opt)) == -1)
        {
            FATAL("ERROR: Call to setsockopt failed (port: %i): %s", port, strerror (errno));
            close (mysocket);
            return -1;
        }
    }
    // initialize data for socket listening...    
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons (port);

    DEBUG("binding port");
    if (bind (mysocket, (struct sockaddr *)addr, sizeof (*addr)) == -1)
    {
        FATAL("ERROR: Error binding socket (port: %i): %s", port, strerror (errno));
        close (mysocket);
        return -1;
    }
    
    DEBUG("setting up listen queue");
    if (listen (mysocket, 5) == -1)
    {
        FATAL("ERROR: Error listening socket (port: %i): %s", port, strerror (errno));
        close (mysocket);
        return -1;
    }
    return mysocket;
}

void
setup_signal_handlers ()
{
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
}
int
main (int argc  UNUSED_SYMBOL, char** argv  UNUSED_SYMBOL)
{
    setup_signal_handlers ();

    fprintf (stderr, "WARNING : daemonisation disabled!!!\n");
    /*
    if (getppid () != 1)
    {
        int forker;
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
    */
  restart:
    // cleanup restart state... 
    // its easy
    g_need2restart = 0;

    if (!get_options ())
        return 1;

    if (!open_log ())
        return 1;
    
    DEBUG ("init userlist");
    g_userlist = calloc (1, sizeof (CLIST));
    if (!g_userlist)
    {
        FATAL("ERROR: Can't allocate memory for g_userlist");
        cleanup ();
        return 1;
    }
    clist_init (g_userlist);    

    DEBUG ("init serverlist");
    g_serverlist = calloc (1, sizeof (CLIST));
    if (!g_serverlist)
    {
        FATAL("ERROR: Can't allocate memory for g_serverlist");
        cleanup ();
        return 1;
    }
    clist_init (g_serverlist);

    DEBUG ("init timelist");
    g_timelist = calloc (1, sizeof (CLIST));
    if (!g_timelist)
    {
        FATAL("ERROR: Can't allocate memory for g_timelist");
        cleanup ();
        return 1;
    }
    clist_init (g_timelist);

    
    // check for port equality or misvalidty, it will also include case when both ports are equal to -1
    if (g_userport == g_serverport)
    {
        FATAL( "ERROR: Invalid port values specified (they are equal)" );
        cleanup ();
        return 1;
    }

    //
    if (g_userport > 0) // i.e. not 0 and not -1
    {
        DEBUG ("init userport");
        usersocket = opensocket (g_userport, &useraddr);
        if (usersocket == -1)
        {
            FATAL( "ERROR: unable to open port for users" );
            cleanup ();
            return 1;
        }
    }
    
    if (g_serverport > 0)
    {
        DEBUG ("init serverport");
        serversocket = opensocket (g_serverport, &serveraddr);
        if (serversocket == -1)
        {
            FATAL( "ERROR: unable to open port for servers" );
            cleanup ();
            return 1;
        }
    }

    // generate server name, if not already set
    // 13 Sep 2002, SteelDen
    // bug fix: if g_serverport = 0 AND g_servname = ""
    // ichat server fails to start
    if (g_serverport > 0
        && !g_servname[0])
    {
        // query system for max 10 interface addresses
        struct ifconf conf;
        struct ifreq req[10];
        int last;
        char hostname[128];

        //
        conf.ifc_len = sizeof (req);
        conf.ifc_req = req;
        
        DEBUG ("query interfaces");
        if (ioctl (serversocket, SIOCGIFCONF, &conf) == -1)
        {
            FATAL("ERROR: Unable to query interface addresses: %s", strerror (errno));
            cleanup ();
            return 1;
        }
    
        // get interface address
        last = conf.ifc_len / sizeof (req[0]);
        if (last == 0)
        {
            FATAL("ERROR: There are no available interface addresses");
            cleanup ();
            return 1;
        }
    
        if (last > 10) 
            last = 10;
        last--;

        DEBUG ("getting hostname");
        // get host name...
        if (gethostname (hostname, sizeof (hostname) - 1) == -1)
        {
            FATAL("ERROR: Can't obtain host name: %s", strerror (errno));
            cleanup ();
            return 1;
        }
        // generate name...
        sprintf (g_servname, "%s/%s", inet_ntoa (((struct sockaddr_in *)&req[last].ifr_addr)->sin_addr) , hostname);
        IMPORTANT("INFO: Generated server name is: %s", g_servname);
    }

    // shit fuck, mother crackers...
    // we dump our pid to the file, so anybody who wants to restart or destroy us can do it easily
    // more over... we need to do unlink to this file in case of exit out...
    // do it always... as signal of successful initialization
    { 
        FILE * pidfile = fopen (g_pidfile, "w+t");
        if (pidfile)
        {
            NOTE("writing to file out PID");
            fprintf (pidfile, "%u\n", getpid ());
            fclose (pidfile);
        }
    }

    //    
    IMPORTANT("Server %s successfully", (g_restarted) ? "restarted" : "started");
    
    if (g_userport != -1)
        IMPORTANT("Listening for users: %s:%i", inet_ntoa (useraddr.sin_addr), ntohs (useraddr.sin_port));

    if (g_serverport != -1)
        IMPORTANT("Listening for servers: %s:%i", inet_ntoa (serveraddr.sin_addr), ntohs (serveraddr.sin_port));

    //print2log( "Ready for queries" );
    // we can set it here... as there is no way to get throught this code without restating...
    g_restarted = 1;

    //
    {
        DEBUG ("server ports poll'em all");
        int pollresult = pollall ();
    
        if (pollresult == -1) // force restart
        {
            DEBUG ("server ports poll'em all = restart(-1)");
            g_need2restart = 1; // allow threads to break upon restart request
            clist_waitempty (g_userlist);
            clist_waitempty (g_serverlist);

            cleanup ();     
            goto restart;
        }
        else
        {
            DEBUG ("server ports poll'em all = exit");
            g_need2exit = 1;
            clist_waitempty (g_userlist);   // like it was above, but on exit request
            clist_waitempty (g_serverlist);
        
            cleanup ();
            return pollresult;
        }
    }
    return 0;       
}

// poll for connections..
// returns 0 or 1 for exit request (success and bad corresponding), and -1 upon restart request
int
pollall (void)
{
    // for now, allow only to items to be used...
    struct pollfd pfd[2];
    int pfdcount = 0;

    //
    if (g_userport != -1)
        pfd[pfdcount++].fd = usersocket;
    
    if (g_serverport != -1)
        pfd[pfdcount++].fd = serversocket;

    //
    while (1)
    {
        // use algo for accepting connections
        // wait for connection, if ready - accept it, otherwise - check for need* vars
        int poller, cnt;

        // reset poll state
        for (cnt = 0; cnt < pfdcount; cnt++)
        {
            pfd[cnt].events = POLLIN;
            pfd[cnt].revents = 0;
        }

        // original idea - poll the socket every 100 ms.
        // but after checking, it was discovered that poll breaks upon
        // signal and returns EINTR... so we can wait for infinite timeout
        // and react only in signal errors
        // QUESTION - what if many threads are active ? which one will catch the signal ?
        // suppose that we need to break sometimes...
        poller = poll (pfd, pfdcount, -1);
        if (poller == -1 && errno != EINTR ) // error condition
        {
            FATAL("ERROR: Can't poll sockets: %s", strerror (errno));
            return 1;
        }
    
        // check for g_need2exit signal...
        // i still can't understand the behaviour with signals and threads...
        if (g_need2exit)
        {
            IMPORTANT("INFO: Detected EXIT request");
            return 0;
        }

        // bad. we received restart signal...
        if (g_need2restart)
        {
            IMPORTANT("INFO: Detected RESTART request");
            return -1; // force restart
        }

        // oki, we are here... so nothing happens, but check for connection
        // if poller (i.e. number of structures accepted) equals 1
        if (poller > 0) // i.e. at least on socket has accepted
        {
            // scan all entries... may be in reverse ? :)
            for (cnt = 0; cnt < pfdcount; cnt++)
            {
                if (pfd[cnt].revents & POLLIN)
                { 
                    // check for mode...
                    if (pfd[cnt].fd == usersocket)
                    {
                        DEBUG ("try to accept client");
                        // try to accept user connection
                        // if can't break all, as its fatal error
                        if (!client_accept (usersocket, 1))
                            return 1;
                    }
                    else if (pfd[cnt].fd == serversocket)
                    {
                        DEBUG ("try to accept server");
                        if (!client_accept (serversocket,0))
                            return 1;
                    }
                }
            }
        }
    }
    // we should never get here
    return 0;
}

// accept new user connection, coming from mainsocket
// returns 1 upon success and 0 on error
int
client_accept (int s, int user)
{
    pthread_t thread; // new thread handle
    
    struct sockaddr_in clientaddr; // client address info
    socklen_t clientaddr_len;

    int ns; // storage for the new socket handle
    
    CLIENT * client; // new user information...
    
    // clear client socket address
    // TODO - why ?
    clientaddr_len = sizeof (clientaddr);
    memset (&clientaddr, 0, clientaddr_len);
    
    // accept connection...
    DEBUG ("client accept()");
    ns = accept (s, (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (ns == -1)
    {
        IMPORTANT("ERROR: Can't accept client connection (%i): %s", user, strerror (errno));
        return 0;
    }
    DEBUG ("client accept() = %d", ns);
    // dump client connection...
    NOTE("INFO: New client connection (%i) from %s:%i",
         user, inet_ntoa (clientaddr.sin_addr), ntohs (clientaddr.sin_port));
        
    // DONE
    // WARN all errors which can occur down will not break program execution
    // and they are fatal only for 
    // create user entry...

    DEBUG ("making new client");
    client = client_create (ns, &clientaddr, (user) ? CLIENT_USER : CLIENT_INSERVER);
    if (client)
    {
        // StaX - 29/01/2002 - some rearrangements with user_clean and user_add    
    
        /*  // add user to the list
        // so it can receive broadcasts... if enabled
        ulist_add( user );

        // try to create new thread for incoming connection handler     
        if( pthread_create( & thread, NULL, user_thread, (void*) user ) != 0 )
        {
        print2log( "ERROR: Can't create user thread" );
        
        // remove user from the list, as thread creation failed
        // this will clean all memory allocated and aslo close accepted socket
        ulist_remove( user );           
        user_clean( user );
        }*/
        DEBUG ("new thread special for client ;]");
        if (pthread_create (&thread, NULL, client_thread, (void*) client) != 0)
        {
            IMPORTANT("ERROR: Can't create client thread (%i)", user);
            client_clean (client);
        }
    }
    else
    {
        IMPORTANT("ERROR: unable to catch client's socket");
        // just close lost socket
        close (ns);
    }
    // return success
    return 1;
}

// used as thread procedure for each client
// take new USER structure as paramater
void *
client_thread (void * parameter)
{
    DEBUG ("i'm threaded");
    CLIENT * client = (CLIENT *)parameter;
    clist_add (client->type == CLIENT_USER ? g_userlist : g_serverlist, client);

    // ALERT
    // make this thread detached...
    // it means that any information related to the
    // thread will be immedialty released after thread completion
    DEBUG ("i detach");
    pthread_detach (pthread_self ());

    // setup socket to nonblocking mode
    // this will immediatly break recv ops or recv ops...
    // fcntl( clientsocket, F_SETFL, O_NONBLOCK );

    // code and loop begins here
    DEBUG ("i'm looping");
    while (client_process (client));

    // id did not want to make a very big if for checking initiali memory allocation
    // by the way, if is a some wrapper against goto, is't it ?

    // make some cleanup code
    if (client->identifier)
        NOTE("INFO: Close connection for %s (reason: %s)", client->identifier, client->reason);
    else
        NOTE("INFO: Close unidentified connection (reason: %s)", client->reason);
    
    clist_remove (client->type == CLIENT_USER ? g_userlist : g_serverlist, client);

    DEBUG ("i'm cleaning up");
    client_clean (client);

    DEBUG ("i'm dead");
    return NULL;
}
