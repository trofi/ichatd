#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

static FILE *          log_file  = NULL;
static enum LOG_LEVEL  log_level = DEBUG_LEVEL;
static int log_write_stdout      = 1;
//#define LOG_MT
#ifdef LOG_MT
#include <pthread.h>
static pthread_mutex_t log_mutex;
#define LOG_INIT    pthread_mutex_init (&log_mutex, NULL)
#define LOG_LOCK    pthread_mutex_lock (&log_mutex)
#define LOG_UNLOCK  pthread_mutex_unlock (&log_mutex)
#define LOG_DESTROY pthread_mutex_destroy (&log_mutex)
#define TID pthread_self ()

#else // LOG_MT
#define NOP while (0) {}
#define LOG_INIT    NOP
#define LOG_LOCK    NOP
#define LOG_UNLOCK  NOP
#define LOG_DESTROY NOP
#define TID getpid ()

#endif // LOG_MT

// returns:
// 0 - error
// 1 - ok

int
open_log (const char * fname)
{
    if (!(log_file = fopen (fname, "a+t")))
    {
        perror ("Error opening log file\n");
        return 1;
    }
    LOG_INIT;
    return 0;
}

// returns:
// previous log level

enum LOG_LEVEL
set_log_level (enum LOG_LEVEL level)
{
    enum LOG_LEVEL prev = log_level;
    log_level = level;
    return prev;
}

int
log_to_stdout (int whether_to_write)
{
    int prev = log_write_stdout;
    log_write_stdout = whether_to_write;
    return prev;
}

void
close_log (void)
{
    if (!log_file)
        return;

    fclose (log_file);
    log_file = NULL;
    LOG_DESTROY;
}

// thread safe logging
void
print2log (enum LOG_LEVEL level, const char * msg, ...)
{
    va_list arglist;
    if (!log_file
        || log_level < level)
        return;

    LOG_LOCK;
    {
        time_t tp = time (NULL);
        char * ts = ctime (&tp);
        // clip finishing \n symbol
        ts[strlen (ts) - 1] = 0;
        // write header
        fprintf (log_file, "%s ichatd[%ld]: ", ts, (long)TID);
        if (log_write_stdout)
            fprintf (stdout,   "%s ichatd[%ld]: ", ts, (long)TID);

        // print the message
        va_start (arglist, msg);
        vfprintf (log_file, msg, arglist);
        va_end (arglist);

        if (log_write_stdout)
        {
            va_start (arglist, msg);
            vfprintf (stdout, msg, arglist);
            va_end (arglist);
        }

        // print the footer
        fputc ('\n', log_file);
        if (log_write_stdout)
            fputc ('\n',   stdout);
        // force log to be updated
        fflush (log_file);
        if (log_write_stdout)
            fflush (stdout);
    }
    LOG_UNLOCK;
}

static const char hex[] = "0123456789ABCDEF";

void
log_print_array (enum LOG_LEVEL level, const char * p, unsigned int len)
{
    // worst case - all chars are unprintable (\xXY)
    // rules: %->%%
    // all the rest - as is
    char * q = (char *)malloc (len*4 + 1);
    if (!q)
    {
        FATAL("ERROR: no mem to print aray");
        return;
    }
    const char * src = p;
    char * dst       = q;
    for (; src < p + len; ++src)
    {
        if (*src == '\0')
        {
            *(dst++) = '\\';
            *(dst++) = '0';
        }
        else if (*src == '%')
        {
            *(dst++) = '%';
            *(dst++) = '%';
        }
        else if (isprint (*src))
        {
            *(dst++) = *src;
        }
        else if (*src == '\r')
        {
            *(dst++) = '\\';
            *(dst++) = 'r';
        }
        else if (*src == '\n')
        {
            *(dst++) = '\\';
            *(dst++) = 'n';
        }
        else if (*src == '\t')
        {
            *(dst++) = '\\';
            *(dst++) = 't';
        }
        else
        {
            *(dst++) = '\\';
            *(dst++) = 'x';
            *(dst++) = hex[((unsigned char)*src) / 16];
            *(dst++) = hex[((unsigned char)*src) % 16];
        }
    }
    *dst = '\0';
    print2log (level, "array(%p) = %s", p, q);
    free (q);
}
