#ifndef __LISTENER_H__
#define __LISTENER_H__

struct tagCLIENTMSG
{
    struct tagCLIENTMSG * next;
    int data_size;
    char data_ptr[0]; //dynalloced
};

enum
{
    CLIENT_USER      = 0,
    CLIENT_INSERVER  = 1,
    CLIENT_OUTSERVER = 2,
};

struct tagCLIENT
{
    struct tagCLIENT    * next;
    int                   type;
    int                   clientsocket;     // socket of client connection
    struct sockaddr_in    clientaddr;       // client info
    // message data...
    char                * dataptr;      // pointer to currently allocated buffer
    int                   data_allocsize;   // size of allocated buffer
    int                   data_msgsize;     // size of message being received
    int                   data_size;        // size of already received data
    int                   waitheader;       // mode. waitheader or receive message rest
    char                * reason;       // break reason text
    // NULL until connection is identified
    char                * identifier;       // connection identifier
    // send queue
    // new messages will fall into the end of queue...
    pthread_mutex_t       sq_mutex;
    struct tagCLIENTMSG * sq;
};

typedef struct tagCLIENT CLIENT, USER, SERVER;

//
struct tagSERVERTIME
{
    struct tagSERVERTIME * next;
    char                 * identifier;      // identity of server
    // last message time
    pthread_mutex_t        time_mutex;
    char                   time[20];
};

typedef struct tagSERVERTIME SERVERTIME;

CLIENT * client_create (int clientsocket, struct sockaddr_in * clientaddr, int type);
void client_clean (CLIENT *);
int client_process (CLIENT *);          // returns TRUE on success, or FALSE on error
void time_clean (SERVERTIME *);

#endif // __LISTENER_H__
