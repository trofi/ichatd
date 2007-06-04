#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "globals.h"
#include "log.h"

static FILE * logfile = NULL;
static pthread_mutex_t log_mutex;
// returns:
// 0 - error
// 1 - ok

int
open_log (void)
{    
    logfile = fopen (g_logfile, "a+t");
    if (logfile)
    {
        pthread_mutex_init (&log_mutex, NULL);
        return 1;
    }
    perror ("Error opening log file\n");
    return 0;
}

void
close_log (void)
{
    if (!logfile)
        return;

    fclose (logfile);
    logfile = NULL;
    pthread_mutex_destroy (&log_mutex);
}

// thread unsafe logging
void
print2log (enum LOG_LEVEL level, const char * msg, ...)
{
    va_list arglist;
    if (!logfile || g_log_level < level)
        return;
    pthread_mutex_lock (&log_mutex);
    {
        time_t tp;
        char * ts;
    
        // get the time and clip finishing \n symbol    
        tp = time (NULL);
        ts = ctime (&tp);
        ts[strlen (ts) - 1] = 0;
    
        // write header
        fprintf (logfile, "%s ichatd[%lx]: ", ts, /*getpid ()*/ pthread_self ());
    
        // print the message
        va_start (arglist, msg);
        {
            vfprintf(logfile, msg, arglist);
        }
        va_end (arglist);
        
        // print the footer
        fputc ('\n', logfile);
    
        // force log to be updated
        fflush (logfile);
    }
    pthread_mutex_unlock (&log_mutex);
}
