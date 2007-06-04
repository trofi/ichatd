#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//for pthread_yield
#define __USE_GNU
#include <pthread.h>

#ifdef LINUX_PPOLL
#define __USE_GNU
#define _GNU_SOURCE
#include <poll.h>
#else
#include <sys/poll.h>
#endif // LINUX_PPOLL

#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "globals.h"
#include "clist.h"
#include "listener.h"
#include "log.h"
#include "rc4.h"

/*
  static void dump_msg(char *ptr, int msglen); 
*/

// return -1 on error, or REVENTS that caused the break...
static int
poll_data (int clientsocket, char** reason, int checkforsendalso, int breakontimeout)
{
    // result of poll request...
    int poller; 
    
    // structure for poller
    // code and loop begins here
    // check for errors and signals
    // wait forever for data
    // but this function breaks on signal
    struct pollfd pfd;

    pfd.fd = clientsocket;
    pfd.events = POLLIN;
    if (checkforsendalso)
        pfd.events |= POLLOUT;

    while (1)
    {    
        pfd.revents = 0;
#ifdef LINUX_PPOLL
        struct timespec ts = {0, 10*1000*1000};
        poller = ppoll (&pfd, 1, &ts, NULL);
#else
        poller = poll (&pfd, 1, -1);
#endif
        if (poller == -1 && errno != EINTR )
        {
            FATAL( "ERROR: Can't poll client socket: %s", strerror( errno ) );
            return -1;
        }
    
        // check signals here
        if (g_need2exit)
        {
            DEBUG("someone triggered exit");
            *reason = "need2exit";
            return -1;
        }
    
        if (g_need2restart)
        {
            DEBUG("someone triggered restart");
            *reason = "need2restart";
            return -1;
        }
        if (poller == 0 && breakontimeout)
        {
            break;
        }
        //  // timeout value
        if ( poller == 0 || errno == EINTR ) 
            continue;

        // check for result event...
        // TODO we had removed check "poller == 1" from down there
        // reason was to check validity only on events set, this will make way to non-threaded version much more easier
        if (/*poller == 1 &&*/ pfd.revents & (POLLIN | POLLOUT))
            break;
        if (pfd.revents & (POLLERR | POLLNVAL))
        {
            DEBUG ("i've got POLLERR or POLLNVAL");
            return -1;
        }
        if (pfd.revents & POLLHUP)
        {
            DEBUG ("i've got POLLHUP");
            break;
        }

        if (pfd.revents)
        {
#define INT_TO_STR(i) (pfd.revents & i) ? #i:""
            DEBUG("unexpected poll event: %x[%s|%s|%s|%s|%s|%s]",
                  pfd.revents,
                  INT_TO_STR(POLLIN),
                  INT_TO_STR(POLLPRI),
                  INT_TO_STR(POLLOUT),
                  INT_TO_STR(POLLERR),
                  INT_TO_STR(POLLHUP),
                  INT_TO_STR(POLLNVAL)
                  );
#undef INT_TO_STR
            // *reason = "unexpected poll event";
            // return -1;
        }   
        // if we do not have data
        //        if( poller != 0 && (poller != 1 || pfd.revents != POLLIN) )
        //        {
        //        }
    }
    return pfd.revents;
}
// return -1 on error, or REVENTS that caused the break...
static int
select_data (int clientsocket, char** reason, int checkforsendalso, int breakontimeout)
{
    // result of select request...
    int poller; 
    fd_set r_set, w_set, e_set;
    int result;
    while (1)
    {
        result = 0;
        FD_ZERO(&r_set);FD_ZERO(&w_set);FD_ZERO(&e_set);
        FD_SET(clientsocket, &r_set);
        FD_SET(clientsocket, &e_set);
        if (checkforsendalso)
        {
            FD_SET(clientsocket, &w_set);
        }

        struct timeval tv = {0, 30*1000};
        poller = select (clientsocket + 1, &r_set, &w_set, &e_set, &tv);
        if (poller == -1 && errno != EINTR )
        {
            FATAL( "ERROR: Can't poll client socket: %s", strerror( errno ) );
            return -1;
        }
    
        // check signals here
        if (g_need2exit)
        {
            DEBUG("someone triggered exit");
            *reason = "need2exit";
            return -1;
        }
    
        if (g_need2restart)
        {
            DEBUG("someone triggered restart");
            *reason = "need2restart";
            return -1;
        }
        if (poller == 0 && breakontimeout)
        {
            break;
        }
        //  // timeout value
        if ( poller == 0 || errno == EINTR ) 
            continue;

        // check for result event...
        // TODO we had removed check "poller == 1" from down there
        // reason was to check validity only on events set, this will make way to non-threaded version much more easier
        if (FD_ISSET(clientsocket, &r_set))
            result |= POLLIN;
        if (FD_ISSET(clientsocket, &w_set))
            result |= POLLOUT;
        if (result)
            return result;
        if (FD_ISSET(clientsocket, &e_set))
            return -1;
        // if we do not have data
        //        if( poller != 0 && (poller != 1 || pfd.revents != POLLIN) )
        //        {
        //        }
    }
    return result;
}

// retursn -1 on error, or size of data that have been read. or 0 - if EAGAIN encountered
static int
read_data (int clientsocket, char* buffer, int bufsize, char** reason)
{
    // receive data
    int recver = recv (clientsocket, buffer, bufsize, 0);
    
    // check for errors
    if (recver == -1)
    {
        if (errno == EINTR)
        {
            // check for MY signals...
            // we will break on all other signals...
            if (g_need2exit)
            {
                *reason = "need2exit";
                return -1;
            }
            else if (g_need2restart)
            {
                *reason = "need2restart";
                return -1;
            }
            else
            {
                // TODO make some investigations here... may we need to fall with 0 result when 
                // unknown signal arrive... or try to loop again, until error or successful read
                *reason = "unknown signal during read";
                return -1;
            }
        }
        else if (errno == EAGAIN) // precheck. i do not use NONBLOCK sockets... yet. socket should be in blocking mode
        {
            recver = 0; // return 0 from read_data...????
        }
        else
        {
            NOTE( "ERROR: Can't read client socket: %s", strerror (errno));
            return -1;
        }
    }
    else if (recver == 0)
    {
        // TODO - why ?
        //if( errno == EINTR )
        //    continue;
        // may be special
        // TODO why ?
        // DONE it's very special... the only reason to read 0 bytes is to try to read them from closed connection
        // but may we need to pass zero bytes to next poll, so we will fall out dueto "invalid poll event" reason
        *reason = "no data";
        return -1;
    }
    
    // continue the loop if recv EAGAIN returns...
    // or if any other error can be here
    //if( recver > 0 )
    //    break;
    //
    //  // switch point
    //}
    
    // now we can return only positive recver values, i.e. read data...
    return recver;
}

// create new user entry
// return NULL on error, and pointer to user on success
CLIENT *
client_create (int clientsocket, struct sockaddr_in * clientaddr, int type)
{
    // alloc memory and check for error
    CLIENT * result = calloc (1, sizeof (CLIENT));
    
    if (!result)
    {
        IMPORTANT( "ERROR: Unable to allocate memory for client" );
        return NULL;
    }
    // make initial allocations
    // look in header for field comments
    result->clientsocket = clientsocket;
    
    // copy client address information
    result->clientaddr = *clientaddr;
    
    // by default, connection is unidentified
    // so make it NULL
    result->identifier = NULL;
    
    // pointer to current data buffer, with advanced... if memory was allocated, points to datadyn, otherwise to data
    // size of currently allocated memory for dataptr    
    result->data_size = 0;
    result->data_msgsize = 0;
    
    // current mode ( 1 if we read header, otherwise read rest of data )
    result->waitheader = 1; 

    // shit fuck. reason of death...
    result->reason = "no reason"; // by default, no reason...    

    // result type...
    result->type = type;

    // allocate initial memory (this should be safe for almost all messages)
    result->dataptr = (char*) calloc (1, result->data_allocsize = g_initialmsgbufsz);
    if (!result->dataptr)
    {
        // free already allocated memory 
        free (result);
        //
        IMPORTANT("ERROR: Unable to allocate initial message buffer");
        return NULL;
    }

    // initialize SQ mutex...
    result->sq = NULL;
    pthread_mutex_init (&result->sq_mutex, NULL);

    // initiali time == ""
    //result->time[0] = 0;
    //pthread_mutex_init( & result->time_mutex, NULL );

    //
    return result;
}

void
client_clean (CLIENT * client)
{
    if (!client)
        return;
    DEBUG("i'm shutting down connection");
    // prevent anybody from sending data to and from this stream
    shutdown (client->clientsocket, SHUT_RDWR); // should #ifdef BDS here?
    DEBUG("i'm closing connection");
    close (client->clientsocket);
    
    DEBUG("i'm freeing data");
    // free memory, if was allocated
    if (client->dataptr)
        free (client->dataptr);
    
    DEBUG("i'm freeing inentifier");
    // free allocated memory string
    if (client->identifier)
        free (client->identifier);
    DEBUG("i'm dtor mutex");
    // delete send queue, if exists...
    pthread_mutex_destroy (&client->sq_mutex);
    DEBUG("i'm freeing queue");
    {
        struct tagCLIENTMSG * msg = client->sq;

        while (msg)
        {
            client->sq = msg->next;
            free (msg);
        }
    }
    // what's it?
    //pthread_mutex_destroy( & client->time_mutex );
    DEBUG("i'm freeing myself");
    free (client);
}

// send herlper
struct tag__SEND_COOKIE
{
    void * skipit;      // skip this server during broadcast, can be NULL
    char * id_to;       // destination of the message
    char * buf1;   // buffer1
    int buf1len;        // length of byffer
    char * buf2;   // buffer 2 (can be NULL)
    int buf2len;        // length of buffer
}; 

static void
add_message (CLIENT * client, struct tag__SEND_COOKIE * cook)
{
    struct tagCLIENTMSG * msg;

    // find message size
    int size = cook->buf1len;
    if (cook->buf2)
        size += cook->buf2len;
    
    // allocate memory
    msg = calloc (1, sizeof (struct tagCLIENTMSG) + size);
    if (!msg)
    {
        IMPORTANT("ERROR: Can't allocate memory for message");
        return;
    }
    
    // copy data
    msg->next = NULL;
    msg->data_size = size;
    memcpy (msg->data_ptr, cook->buf1, cook->buf1len);
    if (cook->buf2)
        memcpy (msg->data_ptr + cook->buf1len, cook->buf2, cook->buf2len);
    
    // add to send queue
    pthread_mutex_lock (&client->sq_mutex);

    if (!client->sq) // add to head, if empty
        client->sq = msg;
    else // scan for last message...
    {
        // find the message with NULL next pointer...
        struct tagCLIENTMSG* tmp = client->sq;
        while (tmp->next)
            tmp = tmp->next;
    
        //
        tmp->next = msg;    
    }

    pthread_mutex_unlock (&client->sq_mutex);
    
    // is it bug?
    /*
      print2log ("EBUG: forwarded to %s %s (%s), %i, %i",
      (client->type == CLIENT_USER) ? "user" : "server",
      client->identifier, cook->id_to, cook->buf1len, cook->buf2len);
    */
}

static int
__sendbroadcast (void* item, void* cookie)
{
    CLIENT * client = (CLIENT*) item;
    struct tag__SEND_COOKIE * cook = (struct tag__SEND_COOKIE *) cookie;
    
    // check for skipper...
    if (cook->skipit
        && cook->skipit == item)
        return 1;
    
    // allow unidentified connection to receive messages only if null session allowed
    if (!client->identifier)
    {
        if (g_enablenulluser)
            add_message (client, cook);
    }
    else
    {
        //if( strcmp( user->identifier, cook->id_from ) != 0 ) // do not send message back to user
        add_message (client, cook);
    }
    
    return 1; // continue traverse
}

static int
__senduser (void * item, void * cookie)
{
    USER * user = (USER *) item;
    struct tag__SEND_COOKIE * cook = (struct tag__SEND_COOKIE *) cookie;
    
    // find the client. initially check temp for head    
    // on each step check for consenteneous identifiers, and if not, advance further until NULL 
    if (user->identifier 
        && strcmp (user->identifier, cook->id_to) == 0)
    {
        add_message( user, cook );
        return 0;
    }
    return 1;
}

// time helpers
void
time_clean (SERVERTIME * st)
{
    pthread_mutex_destroy (&st->time_mutex);
    free (st);
}

SERVERTIME *
time_create( char* server, char* time )
{
    SERVERTIME * st = (SERVERTIME *) calloc (1, sizeof (SERVERTIME));
    if (!st)
    {
        IMPORTANT("ERROR: Can't allocate memory for servertime entry");
        return NULL;
    }
    
    //
    pthread_mutex_init (&st->time_mutex, NULL);
    
    st->identifier = strdup (server); //check 4 oflow
    
    if (time)
        strcpy (st->time, time);
    else
        st->time[0] = 0;
    
    //
    return st;
}

struct tag__CHECKTIME_COOKIE
{
    char* server;
    char* time;
    int result;
};

static int
__checktime (void * item, void * cookie)
{
    struct tag__CHECKTIME_COOKIE * cook = (struct tag__CHECKTIME_COOKIE *) cookie;
    SERVERTIME * st = (SERVERTIME *) item;
    
    // oki, we have found our server...
    if (strcmp (st->identifier, cook->server) == 0)
    {
        pthread_mutex_lock (&st->time_mutex);
    
        if ((cook->result = strcmp (cook->time, st->time)) > 0) // new time is better :)
            strcpy (st->time, cook->time);
        
        pthread_mutex_unlock (&st->time_mutex);
        return 0;
    }
    return 1;
}

// should return 0 if time is lesser or equal to already set, and 1 if it is ok to parse the message
static int
check_and_update_servertime (char * server, char * time)
{
    // check for time value and update if, if lesser
    // add server if time does not exist
    struct tag__CHECKTIME_COOKIE cookie;
    cookie.server = server;
    cookie.time = time;
    
    if (clist_firstthat (g_timelist, __checktime, &cookie)) // was breaked, so we need to check the value...
        return (cookie.result >= 0) ? 1 : 0;
    else
    {
        // no servers found with such ID
        clist_add (g_timelist, time_create (server, time));
        return 1;
    }
}

// data processing functions

// returns the length of generated prefix
static int
make_server_prefix (char * prefix, int additionallength)
{
    struct tm * mt;
    struct timeval tv;
        
    gettimeofday (& tv, NULL);
    mt = gmtime (&tv.tv_sec);
        
    // convert microsecionds to milliseconds and check for overflow (no more than 3 digits)
    if ((tv.tv_usec /= 1000) > 999)
        tv.tv_usec = 999;
    
    return  
        sprintf (prefix, "%i%c%s%c%04i%02i%02i%02i%02i%02i%03li%cFORWARD%c", 
                 strlen (g_servname) + 17 /* length of TIME */ 
                 + 7 /* sizeof FORWARD */ + 3 /* zeroes */ + additionallength,
                 0,
                 g_servname,
                 0, 
                 mt->tm_year + 1900, mt->tm_mon + 1, mt->tm_mday, 
                 mt->tm_hour, mt->tm_min, mt->tm_sec, tv.tv_usec,
                 0,
                 0);
}

// oki, more specific processing...
// 1 - success, 0 - failure
#include <inttypes.h>
#include <netinet/in.h>

static int
user_process_data (USER * user)
{
    const int MAX_IP_LEN  = 128; // more, than 17 :] "xxx.xxx.xxx.xxx\0"
    char      real_prefix[MAX_IP_LEN];
    inet_ntop (AF_INET, &user->clientaddr.sin_addr, real_prefix, sizeof (real_prefix));
    
    // parse packet header
    // if this is a first call (i.e. we have no related user) then create it
    // make new header (for sending)
    // send message to destination (without copying data)
    // free allocated memory
    // set state to read header
    // entry point to parse message text... 
    // parse header
    
    // clip message length and parse fields...
    // TODO (supposed BUG) make checks for length of fields
    int dsize = user->data_msgsize;
    
    char * msg_from, * msg_cmd, * msg_to;
    char * ptr;

 
    msg_from = memchr (user->dataptr, 0, dsize); // this skips message length field
#define CHECK_BOUNDS(p, err_msg)                    \
    if (!p                                          \
        || p >= user->dataptr + user->data_msgsize) \
    {                                               \
        IMPORTANT(err_msg);                         \
        return 0;                                   \
    }

    CHECK_BOUNDS (msg_from, "ERROR: No data in LENGTH field");
    msg_from++; // skip 0 byte
    CHECK_BOUNDS (msg_from, "ERROR: No data after LENGTH field");

    if (strncmp (msg_from, real_prefix, strlen (real_prefix)))
    {
        FATAL("ERROR: mister %s sent packet with FROM field containing <%s>", real_prefix, msg_from);
        return 0;
    }

    dsize -= (msg_from - user->dataptr); // correct msgsize
    
    msg_cmd = memchr (msg_from, 0, dsize);
    CHECK_BOUNDS (msg_cmd, "ERROR: No data in CMD field");
    msg_cmd++;
    CHECK_BOUNDS (msg_cmd, "ERROR: No data after CMD field");

    dsize -= (msg_cmd - msg_from);
    
    msg_to = memchr (msg_cmd, 0, dsize);
    CHECK_BOUNDS (msg_to, "ERROR: No data in TO field");
    msg_to++;
    CHECK_BOUNDS (msg_to, "ERROR: No data after TO field");

    dsize -= (msg_to - msg_cmd);
    
    ptr = memchr (msg_to, 0, dsize);
    CHECK_BOUNDS (ptr, "ERROR: No DATA");
    ptr++;
    CHECK_BOUNDS (ptr, "ERROR: No off DATA");
    // ptr now points to the message start...
    
    // check packet type, if it's not FORWARD...
    if (strcmp (msg_cmd, "FORWARD"))    
    {
        IMPORTANT("ERROR: Unknown command: %s", msg_cmd);
        return 0;
    }
#undef CHECK_BOUNDS

    // create user entry
    // if still undefined, so we have a new user packet
    if (!user->identifier)
    {
        // TODO
        // this code has no sync for multithreaded access
        // but assigment operation is atomic, so this is ok in singe cpu machine
        user->identifier = strdup (msg_from);
        NOTE("INFO: User information discovered: %s", user->identifier);
    }
    // send message here
    {
        // find data length
        size_t msglen = user->dataptr + user->data_msgsize - ptr;
        char prefix[32], prefix2[256];
        size_t prefixlen, prefix2len;
        struct tag__SEND_COOKIE cookie; 

        // send message
        // send prefix of message
        // send message body, take offset as headerlen + sizeof string representation of message length
        cookie.skipit = NULL;
        cookie.id_to = msg_to;

        // prefix for user messages...
        snprintf (prefix, sizeof (prefix) - 1, "%u%cFORWARD%c", msglen + 8, 0, 0); // 8 - size of FORWARD\0
        prefixlen = strlen (prefix);

        cookie.buf1 = prefix;
        cookie.buf1len = prefixlen;
        cookie.buf2 = ptr;
        cookie.buf2len = msglen;
    
        if (msg_to[0] == '*')
        {
            // send to users    
            clist_firstthat (g_userlist, __sendbroadcast, &cookie);
        
            // and to all servers...
            // prefix for server messages
            prefix2len = make_server_prefix (prefix2, user->data_msgsize);
            //print2log( "EBUG: prefix is '%s'", prefix2 );
            
            cookie.buf1 = prefix2;
            cookie.buf1len = prefix2len;
            cookie.buf2 = user->dataptr;
            cookie.buf2len = user->data_msgsize; 
            clist_firstthat (g_serverlist, __sendbroadcast, &cookie);
        }
        else
        {
            // try to send to users... if not found in users... we need to send trough servers
            if (!clist_firstthat (g_userlist, __senduser, & cookie))
            {
                // prefix for server messages
                prefix2len = make_server_prefix (prefix2, user->data_msgsize);
        
                cookie.buf1 = prefix2;
                cookie.buf1len = prefix2len;
                cookie.buf2 = user->dataptr;
                cookie.buf2len = user->data_msgsize; 
        
                // broadcast through all servers...
                clist_firstthat (g_serverlist, __sendbroadcast, &cookie);
            }
        }
    
        if (g_logmessages)
        {
            NOTE("INFO: %s -> %s (recv: %i, message: %i)", msg_from, msg_to, user->data_msgsize, msglen);
        }
    }
    return 1;

}

static int
server_process_data (SERVER * server)
{
    // clip message length and parse fields...
    // TODO (supposed BUG) make checks for length of fields
    char * msg_server   = memchr (server->dataptr, 0, server->data_msgsize) + 1;
    char * msg_time     = memchr (msg_server, 0, server->data_msgsize) + 1;
    char * msg_query    = memchr (msg_time, 0, server->data_msgsize) + 1;
    char * ptr          = memchr (msg_query, 0, server->data_msgsize) + 1;

    //
    /*    print2log( "EBUG: '%s'", msg_server );
      print2log( "EBUG: '%s'", msg_time );    
      print2log( "EBUG: '%s'", msg_query );*/

    // oki... now we should try to check commands...
    if (strcmp (msg_query, "LOGIN") == 0)
    {
        // oki, get and check the password...
        size_t passlen = server->data_msgsize - (ptr - server->dataptr);
    
        if (passlen != strlen (g_servpass) || strncmp (g_servpass, ptr, passlen)) // != 0
        {
            IMPORTANT("ERROR: Invalid password <%s> from '%s'", ptr, msg_server);
            return 0;
        }
    
        // we are in success...
        if (!server->identifier)
        {
            server->identifier = strdup (msg_server);
            NOTE("INFO: Server information discovered: %s", msg_server);
        }
    
        // update time status
        // need multithread lock...
        //pthread_mutex_lock( & server->time_mutex );
        //strcpy( server->time, msg_time );
        //pthread_mutex_unlock( & server->time_mutex );
        check_and_update_servertime (msg_server, msg_time);
    
        //
        if (g_logmessages)
            NOTE( "INFO: %s, LOGIN (time: %s, recv: %i)", msg_server, msg_time, server->data_msgsize );
    }
    else if (strcmp (msg_query, "FORWARD") == 0)
    {
        // check if we already identified
        if (!server->identifier)
        {
            IMPORTANT ("WARN: non-login message from unidentified server");
            return 1;   
        }
    
        // check message time
        // it will return 0 if no server 
        if (!check_and_update_servertime (msg_server, msg_time))
        {
            NOTE ( "WARN: message skipped due to timedist" );
            // no error, just timemismatch
            // skip this message
            return 1;
        }
    
        // gook. parse the message and send it further...
        /* 
           logic is:
        
           if dest is *
           send to all clients and to all servers, but sender
        
           if dest is name,
           send to named client, if can't
           send to all servers, but sender
        */
        {
            // clip message length and parse fields...
            // TODO (supposed BUG) make checks for length of fields
            char * msg_from = memchr (ptr, 0, server->data_msgsize) + 1; // this skips message length field
            char * msg_cmd  = memchr (msg_from, 0, server->data_msgsize) + 1;
            char * msg_to   = memchr (msg_cmd, 0, server->data_msgsize) + 1;
            char * ptr2     = memchr (msg_to, 0, server->data_msgsize) + 1;
        
            // find data length
            int msglen = server->data_msgsize - (ptr2 - server->dataptr);
            char prefix[32];
            int prefixlen;
            struct tag__SEND_COOKIE cookie; 
    
            // prepare cookie
            cookie.skipit = server;
            cookie.id_to = msg_to;

            // prefix for user messages...
            prefixlen = sprintf (prefix, "%u%cFORWARD%c", msglen + 8, 0, 0); // 8 - size of FORWARD\0

            cookie.buf1 = prefix;
            cookie.buf1len = prefixlen;
            cookie.buf2 = ptr2;
            cookie.buf2len = msglen;
    
            if (msg_to[0] == '*')
            {
                // first of all, send to all users...
                clist_firstthat (g_userlist, __sendbroadcast, &cookie);
        
                // and to all servers, but specified
                // all we need is to broadcast the message, so no prefixes are required
                cookie.buf1 = server->dataptr;
                cookie.buf1len = server->data_msgsize; 
                cookie.buf2 = NULL;
        
                clist_firstthat (g_serverlist, __sendbroadcast, &cookie);
            }
            else
            {
                // try to send to users... if not found in users... we need to send trough servers
                if (!clist_firstthat (g_userlist, __senduser, &cookie))
                {
                    // no prefixes again
                    cookie.buf1 = server->dataptr;
                    cookie.buf1len = server->data_msgsize; 
                    cookie.buf2 = NULL;
        
                    // broadcast through all servers...
                    clist_firstthat (g_serverlist, __sendbroadcast, &cookie);
                }
            }
    
            // log message
            if (g_logmessages)
                NOTE ("INFO: %s, FORWARD from %s, %s -> %s (time: %s, recv: %i, message: %i)",
                      server->identifier,
                      msg_server,
                      msg_from,
                      msg_to,
                           msg_time,
                      server->data_msgsize, msglen);
        }
    }
    else
    {
        NOTE ("ERROR: Unknown query from server '%s' (come from '%s'): %s",
              msg_server,
              (server->identifier) ? server->identifier : "unidentified",
              msg_query);
        return 0;
    }    
    //    
    return 1;
}

// parse read data..
// returns 0 on error, and 1 on success
static int
client_process_data (CLIENT * client)
{
    // oki, we have a data on the line...
    // check mode
    // select by state  
    // we need this label to be able to skip initial read...
  analysis:
    if (client->waitheader) // we have no current buffer, so read the header...
    {
        // read 12 bytes (make limit tuned) (we really read much more.. thats bad)
        // check it has a string
        // if not, exit thread with state BAD HEADER
        // check message length
        // if too long, exit thread with state MESSAGE TOO LONG
        // allocate memory
        // copy read rest to allocated buffer
        // set state to read packet 
        
        // check enough message size       
        if (client->data_size < g_minhdrlen) // make this number adjustable (i.e. length number must fit here)
            return 1; // we have no header, process can go further

        // check if have a string in 12 letters...
        if (memchr (client->dataptr, 0, g_minhdrlen)) 
        {
            // find total message length... (really in bytes)
            // count size of length string itself
            // count FOUR zeroes for asciiz strings in header...
            //datamax = atoi( data ) + 4 + strlen( data );
            client->data_msgsize = atoi (client->dataptr) + strlen (client->dataptr) + 1;

            // check for maximum message size
            if (client->data_msgsize > g_maxmsgsz)
            {
                IMPORTANT ("ERROR: The message is very big: %i bytes. Discarding", client->data_msgsize);
                return 0;
            }

            // check memory we have for message
            if (client->data_msgsize > client->data_allocsize)
            {
                // allocate memory enough to hold new message
                char * data = (char*) calloc (1, client->data_msgsize);
                if (!data)
                {
                    IMPORTANT("ERROR: No memory to hold received message, %i bytes",
                              client->data_msgsize);
                    return 0;
                }
            
                // copy already received data into the new buffer
                memcpy (data, client->dataptr, client->data_size);

                // free current buffer
                free (client->dataptr);
            
                // make new buffer current
                client->dataptr = data;
            }
            
            // state now became READ PACKET
            client->waitheader = 0;     
        }
        else 
        {
            // too bad... we are wating for header, but no number in first bytes...
            // so be crazy
            client->reason = "no message length in header";
            return 0;
        }
    }
    
    // if we have enough info in the buffer to parse
    if (client->data_size >= client->data_msgsize)
    {
        // oki, actual processing here...
        if (!((client->type == CLIENT_USER) ? user_process_data (client) : server_process_data (client)) )
            return 0;

        // set mode to waitheader
        client->waitheader = 1;
        
        // oki. check if we have more data in the buffer
        if (client->data_size > client->data_msgsize)
        {
            // copy rest of data in the beginning of buffer
            // SHIT FUCK - it was an example of an "professional" error. Stax - you are a cretin...
            memmove (client->dataptr, client->dataptr + client->data_msgsize,
                     client->data_size - client->data_msgsize);
            
            // correct data_size value, so its reftlect valid size of semireceived data
            client->data_size -= client->data_msgsize;

            // go directly to header analisys
            goto analysis;
        }
        else
            // only one reason... its equal
            client->data_size = 0;
    }
    
    // continue processing...
    return 1; 
}

// return 0 on error and 1 - on success
int
client_process (CLIENT * client)
{
    /*
      blocking mode:
    
      poll for data without timeout
      check if have something to read
      read send 
      check if have something to send (there are mesages in the queue)
      send it
    */
    int rpc = /*poll_data*/
              select_data
              (client->clientsocket, &client->reason, 0, 1);
    if( rpc == - 1)
        return 0;
    
    // check for data readyness
    if (rpc & POLLIN)
    {
        // read chunk of data
        int size = ((client->waitheader) ? client->data_allocsize : client->data_msgsize) - client->data_size;
    
        // TODO if investiagtion upon signal processing (see read_data)
        // will lead to returning 0 value upon unknown signal we need to make a check here for it
        // skipping null read_data call
        // even now, returning 0 from read_data will perform nothing... :?
        int rr = read_data (client->clientsocket, client->dataptr + client->data_size, size, & client->reason);
        if (rr == -1)
            return 0;
    
        if (rr) // we have read something
        {
            // correct message size     
            client->data_size += rr;
        
            // call the data processor
            if (!client_process_data (client))
                return 0;
        }
    }

    // the next check required only in case of non-blocking sockets...   
    // try to send messages
    {
        /*
      blocking mode:
        
      we will send message here, i.e. delete it from the queue and block until it would be completly sent
        
      non-blocking mode:
        
      we should try to send message until it will go... so we need to store in the queue
      with send_bytes_counter and try to send until the counter became zero
    */
        struct tagCLIENTMSG * msg = NULL;
    
        // check non-empty queue... and extract message
        pthread_mutex_lock (&client->sq_mutex);
    
        if (client->sq)
        {
            msg = client->sq;
            client->sq = msg->next;
        }
    
        pthread_mutex_unlock (&client->sq_mutex);
    
        // we have something to send...
        if (msg)
        {
            // try to send data...
            if (send (client->clientsocket, msg->data_ptr, msg->data_size, 0) == -1)
            {
                // on failure, free the memory
                free (msg);
                //
                IMPORTANT("ERROR: Can't send to client socket: %s", strerror (errno));
                return 0;
            }
            // anyway, we need to free the memory
            free (msg);
        }
    }
    
    // success by default
    return 1;
}

/* // too expensive
  static void
  dump_msg(char *ptr, int msglen)
  {
  int i;
  struct rc4_state s;
  char *buffer, *p;
  
  if (msglen < 1)
  return;
  
  buffer = (char *)calloc(1, msglen);
  p = buffer;
  
  rc4_setup( &s, "tahci", 5);
  memcpy(buffer, ptr, msglen);
  rc4_crypt( &s, buffer,  msglen);
    
  print2log( "Got message:" );
  for (i = 0; i < msglen; i++){
  if (isprint(p[i]))
  print2log("%c", p[i]);
  else 
  print2log(".(%x)", (int)p[i]);
  }
  print2log("\n\n");
  free(buffer);
  }
*/
