#ifndef __DEBUG_H__
#define __DEBUG_H__

// Set it when need to debug code in poller.c
//#define ENABLE_POLL_DEBUG 1
//                                in config.c
//#define ENABLE_CONFIG_DEBUG 1

// for poller.c
#ifdef ENABLE_POLL_DEBUG
#    define POLL_DEBUG(fmt, args...) DEBUG(fmt, ##args)
#else // ENABLE_POLL_DEBUG
#    define POLL_DEBUG(fmt, args...) do {} while (0);
#endif // ENABLE_POLL_DEBUG

// for config.c
#ifdef ENABLE_CONFIG_DEBUG
#    define CONFIG_DEBUG(fmt, args...) fprintf(stderr, fmt "\n", ##args)
#else // ENABLE_CONFIG_DEBUG
#    define CONFIG_DEBUG(fmt, args...) do {} while (0);
#endif // ENABLE_CONFIG_DEBUG

#endif // __DEBUG_H__
