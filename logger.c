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
                char int_to_str_arg_buf[MAX_ARG_SIZE];
                sprintf(int_to_str_arg_buf, "%d", va_arg(args, int));
                formatted_msg = strcat(formatted_msg, int_to_str_arg_buf);
                // keep appending to the end of the new string
                j += strlen(int_to_str_arg_buf);
                break;
            case 's':
                char * str_arg = va_arg(args, char *);
                formatted_msg = strcat(formatted_msg, str_arg);
                j += strlen(str_arg);
                break;
            case 'p':
                char ptr_to_str_arg_buf[MAX_ARG_SIZE];
                sprintf(ptr_to_str_arg_buf, "%p", va_arg(args, void *));
                formatted_msg = strcat(formatted_msg, ptr_to_str_arg_buf);
                j += strlen(ptr_to_str_arg_buf);
                break;
            }

        } else {
            formatted_msg[j] = raw_msg[i];
            j++;
        }
        // finally terminate the string if we havent terminated already
        formatted_msg[j] = '\0';
    }
    return formatted_msg;
}

void logger(LogLevel level, const char *message, ...)
{
    if (log_level < level)
    {
        return;
    }

    // Declare a va_list variable to manage the variable arguments
    va_list args;

    va_start(args, message);

    char new_msg[MAX_MSG_SIZE];
    char *formatted_msg = format_string(message, args, new_msg);

    va_end(args);

    char * level_str;
    switch (level) {
        case DEBUG:
            level_str = "DEBUG";
            break;
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
    printf("[%s]: %s\n", level_str, formatted_msg);
}