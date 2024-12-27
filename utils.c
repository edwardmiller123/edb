#include <string.h>

#include "utils.h"

// Returns true if the target string contains the given prefix
bool has_prefix(char *target, char *prefix)
{
    return strncmp(target, prefix, strlen(prefix)) == 0;
}