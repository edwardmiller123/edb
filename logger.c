#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#include "logger.h"

#define MAX_MSG_SIZE 100
#define MAX_ARG_SIZE 32

static LogLevel log_level = INFO;

void set_log_level(LogLevel level) {
    log_level = level;
}

char *format_string(const char *raw_msg, va_list args, char *formatted_msg)
{
    int j = 0;
    for (int i = 0; i < strlen(raw_msg); i++)
    {
        if (raw_msg[i] == '%')
        {
            // terminate the new string before we append to it
            formatted_msg[j] = '\0';
            i++;

            switch (raw_msg[i])
            {
            case 'd':
                char str_arg_buf[MAX_ARG_SIZE];
                sprintf(str_arg_buf, "%d", va_arg(args, int));
                formatted_msg = strcat(formatted_msg, str_arg_buf);
                // keep appending to the end of the new string
                j += strlen(str_arg_buf);
                break;
            case 's':
                formatted_msg = strcat(formatted_msg, va_arg(args, char *));
                j += strlen(va_arg(args, char *));
                break;
            }

        } else {
            formatted_msg[j] = raw_msg[i];
            j++;
        }
    }
    return formatted_msg;
}

void logger(LogLevel level, const char *message, ...)
{
    // Declare a va_list variable to manage the variable arguments
    va_list args;

    va_start(args, message);

    char new_msg[MAX_MSG_SIZE];
    char *formatted_msg = format_string(message, args, new_msg);

    va_end(args);

    if (log_level < level)
    {
        return;
    }

    time_t now;
    time(&now);
    char * human_time = ctime(&now);

    // remove the reailing newline
    for (int i = 0; i < strlen(human_time); i++) {
        if (human_time[i] == '\n') {
            human_time[i] = '\0';
        }
    }

    char * level_str;
    switch (level) {
        case INFO:
            level_str = "INFO";
            break;
        case WARN:
            level_str = "WARN";
            break;
        case ERROR:
            level_str = "ERROR";
            break;
    }
    printf("%s [%s]: %s\n", human_time, level_str, formatted_msg);
}