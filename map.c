#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "map.h"
#include "logger.h"

// Creates a instance of an empty map of string to void pointer
// TODO: need to handle collisions
Map *new_map()
{
    Map *map = malloc(sizeof(Map));
    if (map == NULL)
    {
        logger(ERROR, "Failed to allocate heap memory. ERRNO: %d", errno);
        return NULL;
    }

    map->size = 0;

    // set all elements to 0 to signifiy the map is empty
    memset(map->data, 0, MAX_MAP_SIZE);
    return map;
}

// Hashes the given string using the djb2 algorythm
int hash(char *str)
{
    int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// Adds an element to the map. Returns -1 if the map is full.
int m_set(Map *map, char *key, void *elem)
{
    int idx = hash(key) % MAX_MAP_SIZE;
    if (map->size == MAX_MAP_SIZE)
    {
        return -1;
    }
    map->size++;
    map->data[idx] = elem;
    return 0;
}

void *m_get(Map *map, char *key)
{
    int idx = hash(key) % MAX_MAP_SIZE;
    return map->data[idx];
}

// Removes an element from the map. Returns -1 if the map is empty.
int m_remove(Map *map, char *key)
{
    if (map->size == 0)
    {
        return -1;
    }
    int idx = hash(key) % MAX_MAP_SIZE;
    map->data[idx] = NULL;
    map->size--;
    return 0;
}

// Returns 1true if the map is full
bool m_is_full(Map *map)
{
    return map->size >= MAX_MAP_SIZE;
}
