#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>

#include "misc.h"

char *
IC_strdup (const char * str)
{
    size_t len = strlen (str);
    char * new_str = malloc (sizeof (char) * (len + 1));
    if (new_str == 0)
        return new_str;
    memcpy (new_str, str, len);
    new_str[len] = '\0';
    return new_str;
}

int
IC_bind_server_socket (const char * laddr, int port)
{
    struct sockaddr_in a;
    int s;
    int yes;

    if (port < 0 || port > 65535)
        goto e_no_sock;

    if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        goto e_no_sock;
    
    yes = 1;
    
    if (setsockopt  (s, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof (yes)) < 0)
        goto e_bad_init;
    
    memset (&a, 0, sizeof (a));
    a.sin_port = htons (port);
    a.sin_family = AF_INET;
    
    if( !inet_aton( laddr, (struct in_addr *) &a.sin_addr.s_addr) )
        goto e_bad_init;
    
    bind (s, (struct sockaddr *) &a, sizeof (a));
    listen (s, 10);
    return s;

  e_bad_init:
    close (s);
  e_no_sock:
    return -1;
}

int
IC_nonblock_connect (const char * raddr, int port)
{
    struct sockaddr_in a;
    int s;
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
        goto e_no_sock;

    if (fcntl (s, F_SETFD, O_NONBLOCK) == -1)
        goto e_bad_init;
    
    memset (&a, 0, sizeof (a));
    a.sin_port = htons (port);
    a.sin_family = AF_INET;

    if (inet_aton (raddr, (struct in_addr *) &a.sin_addr.s_addr) == 0)
        goto e_bad_init;

    if (connect (s, (struct sockaddr *) &a, sizeof (a)) < 0
        && errno != EINPROGRESS)
	goto e_shut;
    return s;

  e_shut:
    shutdown (s, SHUT_RDWR);
  e_bad_init:
    close (s);
  e_no_sock:
    return -1;
}

size_t number_len (size_t number)
{
    if (number < 10)
        return 1;
    return 1 + number_len (number / 10);
}

long long
GetTimerMS(void)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}
