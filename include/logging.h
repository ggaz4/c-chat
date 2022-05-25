#ifndef C_CHAT_LOGGING_H
#define C_CHAT_LOGGING_H

#define DEBUG_LEVEL   2
#define INFO_LEVEL    4
#define ERROR_LEVEL   8

extern int LOG_LEVEL;


void log_format(const char *level, const char *message, va_list args);

void log_debug(const char *message, ...);

void log_info(const char *message, ...);

void log_error(const char *message, ...);

void log_with_errno(char *message, ...);

void log_usage(const char *, const char *);

#endif //C_CHAT_LOGGING_H
