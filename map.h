
#ifndef __MAP_H__
#define __MAP_H__

#include <stdint.h>
#include <stdbool.h>

#include "slice.h"

/*
    Maps or Hash tables give a O(1) access to almost any element of the table.
    Only a limited number of elements require additional processing in case of
    hash collision. This implementation uses a linked list for collisions.

    The table contains only void pointers to preallocated keys and data:
        key <- hash value -> data

    It does not require that keys are of any particular type. In fact, keys
    are never directly accessed and key "pointers" could also be used to hold
    any type that can fit in a void pointer.

    The hash value is just derived from the key pointer value, by calling the
    hash function passed to the new_map when the map was created. If NULL was
    given as hash function to new_map, hashing is done by using directly the
    key pointer cast as uint64_t. In this case and if keys are real pointers to
    memory objects, this assumes that each key object is stored at a different
    address that will never change afterwards.

    Similarly, the data value is never directly accessed, and therefore the
    data pointer may contain any value that fits in a void pointer, in addition
    to pointing to an object in memory. If it points to an address in memory,
    the object type can be anything and may even vary between entries (the
    caller has to manage how to deal with those various objects just based on
    their pointer).
*/

typedef struct map map_t;

// hash function used for keys. The hash value must fit in a uint64_t.
typedef uint64_t (*hash_fct)( const void *key );

// key identical function. It must return true if the 2 keys are considered
// the same or false otherwise.
typedef bool (*same_fct)( const void *key1, const void *key2);

// allocate a new map. If size is 0, the table will be allocated the first time
// an enrey is inserted, otherwise a table is preallocated, sufficient for at
// least the requested size entries. The collisions argument gives a threshold
// for resizing automatically the table if the number of collisions reaches
// that threshold. The minimum threshold accepted is 4. If the hash function is
// null, the hash value of a key is just its pointer cast as a size_t, which
// may be fine for some cases. If the hash function is not null a same function
// must be provided as well to tell whether two object keys are the same.
extern map_t *new_map( hash_fct hash, same_fct same,
                                        uint32_t size, uint32_t collisions );

// free an existing map. If needed, keys and data must be freed separately (see
// map_process).
extern int map_free(map_t *map );

// insert a new entry in the map. It returns true if the entry was inserted, or
// false, which can only happen if an entry already exists for that same key
// (according to the result of calling same_fct if given to new_map) or if the
// map requires a re-allocation that failed (no memory).
extern bool map_insert_entry( map_t *map, const void *key, const void *value );

// delete an existing entry in the map. It returns false if the entry did not
// exist or true otherwise.
extern bool map_delete_entry( map_t *map, const void *key );

// return the data pointer associated with the key. It returns NULL if the key
// was not found in the map, otherwise the associated value.
extern const void *map_lookup_entry( const map_t *map, const void *key );

// return the current number of entries in map
extern size_t map_len( const map_t *map );

// execute the passed function for all entries associated with the given key.
// This is useful for the very rare case where collisions are created on
// purpose by giving the same key for different values, which is only possible
// if the same_fct function given to new_map always returns false, regardless
// the keys. In that case a regular call to map_lookup_entry always fails as
// well and the only way to find out the values for a given key is to call
// map_process_entries_for_key. Since same_fct is useless, key pointers are
// directly compared, and only entries matching the requested key pointer are
// returned. When the last value associated with the key has been processed,
// an extra call to multiple_entry_fct is done with NULL data to indicate the
// end of the list.
typedef void (*multiple_entry_fct)(const void *key, const void *data,
                                   void *context);
extern void map_process_entries_for_key( const map_t *map, const void *key,
                                         multiple_entry_fct proc,
                                         void *context );

// execute the passed function for all entries in the map. This could be used
// to free keys and data allocated in memory before freeing the whole map. The
// order in which keys are processed is not guaranted, although all entries at
// the same index (i.e. all colliding entries found at that index) are always
// processed in sequence. This can be used to get the collision distribution.
// The procees is stopped as soon as entry_process_fct returns true.
// Warning: as it is done by sweeping the whole table, it is pretty slow.
typedef bool (*entry_process_fct)(uint32_t entry_index,
                                  const void *key, const void *data,
                                  void *context);
extern void map_process_entries( const map_t *map,
                                 entry_process_fct proc, void *context );

// return a slice with all keys in the map, or NULL in case of failure. If the
// argument cmp is not NULL, keys are sorted otherwise keys are NOT stored in
// any particular order. Keys in the slice are just the void * that were
// provided in map_insert_entry. After use the returned slice must be freed by
// calling slice_free.
extern slice_t *map_keys( const map_t *map, comp_fct cmp );

// stats for checking usage and collisions
extern void map_stats( const map_t *map );

#endif /* __MAP_H__ */
