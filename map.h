#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

#define MAX_MAP_SIZE 100
#define MAX_KEY_SIZE 50

// Map of strings to void pointers
typedef struct Map {
    int size;
    void * data[MAX_MAP_SIZE];
} Map;

// Creates a instance of an empty map of string to void pointer
Map *new_map();

// Adds an element to the map. Returns -1 if the map is full.
int m_set(Map * map, char * key, void * elem);

void * m_get(Map * map, char * key);

// Removes an element from the map. Returns -1 if the map is empty.
int m_remove(Map * map, char * key);

// Returns true if the map is full
bool m_is_full(Map *map);

// Returns true if the map is empty
bool m_is_empty(Map *map);

#endif
