#ifndef LOGGER_H
#define LOGGER_H

typedef enum LogLevel {
    ERROR,
    WARN,
    INFO,
    DEBUG
} LogLevel;

void set_log_level(LogLevel level);

void logger(LogLevel level, const char *message, ...);

#endif
