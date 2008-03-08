#ifndef __LOG_H__
#define __LOG_H__

enum LOG_LEVEL {
    NONE_LEVEL = 0,
    FATAL_LEVEL,
    WARN_LEVEL,
    NOTE_LEVEL,
    DEBUG_LEVEL
};

// 0    - all ok
// else - error
int open_log (const char * fname);
void close_log (void);

// returns pre state
int log_to_stdout (int whether_to_write);

enum LOG_LEVEL set_log_level (enum LOG_LEVEL level);
void print2log (enum LOG_LEVEL level, const char * msg, ...);

#define FATAL(fmt, args...)     print2log(FATAL_LEVEL, fmt, ##args)
#define WARN(fmt, args...)      print2log(WARN_LEVEL, fmt, ##args)
#define NOTE(fmt, args...)      print2log(NOTE_LEVEL, fmt, ##args)
#define DEBUG(fmt, args...)     print2log(DEBUG_LEVEL, fmt, ##args)

void log_print_array (enum LOG_LEVEL level, const char * p, unsigned int len);

#endif // __LOG_H__
