#include "globals.h"
#include "log.h"

int g_reuseaddr = 1;
int g_userport = 6666;
int g_serverport = 6667;
char g_logfile[256];
unsigned int  g_log_level = IMPORTANT_LEVEL;
char g_pidfile[256];

volatile sig_atomic_t g_need2restart = 0;
volatile sig_atomic_t g_need2exit    = 0;
volatile sig_atomic_t g_restarted    = 0;

// lists
CLIST * g_userlist   = NULL;
CLIST * g_serverlist = NULL;
CLIST * g_timelist   = NULL;

int  g_enablenulluser = 0;
int  g_minhdrlen = 12;
int  g_initialmsgbufsz = 1024;
int  g_maxmsgsz = 8192; //lesser a bit ??(kills windoze iChat-1.21b6)
int  g_logmessages = 0;
char g_servpass[128];
char g_servname[128];
