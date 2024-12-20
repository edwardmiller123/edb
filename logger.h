#ifndef LOGGER_H
#define LOGGER_H

typedef enum LogLevel {
    ERROR,
    WARN,
    INFO,
} LogLevel;

void logger(LogLevel level, const char *message, ...);

#endif
