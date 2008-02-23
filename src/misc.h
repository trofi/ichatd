#ifndef __MISC_H__
#define __MISC_H__

// some useful macros

#ifdef __GNUC__
#define UNUSED_SYMBOL __attribute__((unused))
#else
#define UNUSED_SYMBOL
#endif

// some macros

#define min(a,b) ((a) < (b) ? (a) : (b))

// some useful funs

// C like
char * IC_strdup (const char * str);

// socket aware
int IC_bind_server_socket (const char * laddr, int port);
int IC_nonblock_connect (const char * raddr, int port);

// time stuff
long long GetTimerMS(void);

#endif // __MISC_H__
