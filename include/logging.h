#ifndef C_CHAT_LOGGING_H
#define C_CHAT_LOGGING_H

void log_info(const char *, ...);

void log_with_errno(const char *, ...);

void log_usage(const char *, const char *);

#endif //C_CHAT_LOGGING_H
