#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <signal.h>
#include "clist.h"

extern int g_reuseaddr;
extern int g_userport;
extern int g_serverport;
extern char g_logfile[256];
extern unsigned int  g_log_level;
extern char g_pidfile[256];

extern volatile sig_atomic_t g_need2restart;
extern volatile sig_atomic_t g_need2exit;
extern volatile sig_atomic_t g_restarted;

// lists
extern CLIST * g_userlist;
extern CLIST * g_serverlist;
extern CLIST * g_timelist;

extern int  g_enablenulluser;
extern int  g_minhdrlen;
extern int  g_initialmsgbufsz;
extern int  g_maxmsgsz;
extern int  g_logmessages;
extern char g_servpass[128];
extern char g_servname[128];

#endif // __GLOBALS_H__
