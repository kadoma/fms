
#ifndef __LOGGING_H__
#define __LOGGING_H__ 1

#include <stdint.h>

typedef enum{
    WR_LOG_NONE = 0,
    WR_LOG_CRITICAL,   /* fatal errors */
    WR_LOG_ERROR,      /* major failures (not necessarily fatal) */
    WR_LOG_WARNING,    /* info about normal operation */
    WR_LOG_NORMAL,     /* Normal information */
    WR_LOG_DEBUG,      /* all other junk */
}wr_loglevel_t;

#define WR_LOG_MAX WR_LOG_DEBUG

extern wr_loglevel_t wr_log_loglevel;

#define wr_log(dom, levl, fmt...) do{                          \
  if (levl <= wr_log_loglevel)                                  \
  _wr_log (dom, __FILE__, __FUNCTION__, __LINE__, levl, ##fmt); \
} while(0)

void
wr_log_logrotate(int signum);

int32_t
_wr_log(const char *domain,
    const char *file,
    const char *function,
    int32_t line,
    wr_loglevel_t level,
    const char *fmt, ...);

int32_t
wr_log_init(const char *filename);

wr_loglevel_t
wr_log_get_loglevel(void);

void
wr_log_set_loglevel(wr_loglevel_t level);

#endif /* __LOGGING_H__ */
