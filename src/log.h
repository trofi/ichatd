#ifndef __LOG_H__
#define __LOG_H__

int open_log (void);
void close_log (void);
enum LOG_LEVEL {
    NONE_LEVEL = 0,
    FATAL_LEVEL,
    IMPORTANT_LEVEL,
    NOTE_LEVEL,
    DEBUG_LEVEL
};

void print2log (enum LOG_LEVEL level, const char * msg, ...);

#define FATAL(fmt, args...)     print2log(FATAL_LEVEL, fmt, ##args)
#define IMPORTANT(fmt, args...) print2log(IMPORTANT_LEVEL, fmt, ##args)
#define NOTE(fmt, args...)      print2log(NOTE_LEVEL, fmt, ##args)
#define DEBUG(fmt, args...)     print2log(DEBUG_LEVEL, fmt, ##args)

#endif // __LOG_H__
