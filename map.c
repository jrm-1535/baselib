
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "slice.h"
#include "_slice.h"
#include "map.h"

typedef struct _map_entry {
    struct _map_entry *next;    // linked list in case of collisions
    const void         *key;    // NULL for an unused entry in table
    const void         *data;
    uint64_t            hash;
} map_entry_t;

struct map {
    map_entry_t         *table;
    hash_fct            hash_f;
    same_fct            same_f;
    uint32_t            nb;
    uint32_t            allocated;  // prime number of allocated entries
    uint64_t            n2power;    // 2 power immediately above allocated
    uint32_t            max_collision;
    uint32_t            threshold;
};

#define MIN_POWER       3           // start at 2^3
#define MIN_SIZE        (1<<MIN_POWER)
#define MIN_COLLISIONS  4           // Allways accept at least MIN_COLLISIONS

static uint32_t get_prime( uint32_t new_size, uint64_t *n2power )
{
    assert( new_size >= MIN_SIZE );
    static const uint64_t greatest_prime[] = {
        /* start with prime for MIN_SIZE */
        7,          /* 8 */
        13,         /* 16 */
        31,         /* 32 */
        61,         /* 64 */
        127,        /* 128 */
        251,        /* 256 */
        509,        /* 512 */
        1021,       /* 1024 */
        2039,       /* 2048 */
        4093,       /* 4096 */
        8191,       /* 8192 */
        16381,      /* 16384 */
        32749,      /* 32768 */
        65521,      /* 65536 */
        131071,     /* 131072 */
        262139,     /* 262144 */
        524287,     /* 524288 */
        1048573,    /* 1048576 */
        2097143,    /* 2097152 */
        4194301,    /* 4194304 */
        8388593,    /* 8388608 */
        16777213,   /* 16777216 */
        33554393,   /* 33554432 */
        67108859,   /* 67108864 */
        134217689,  /* 134217728 */
        268435399,  /* 268435456 */
        536870909,  /* 536870912 */
        1073741789, /* 1073741824 */
        2147483647, /* 2147483648 */
        4294967291, /* 4294967296 */
        8589934592  /* sentinel above max uint32_t */
    };

    unsigned int i;
    for ( i = 1; i < sizeof(greatest_prime)/sizeof(uint64_t); ++i ) {
        if ( (uint64_t)new_size <= greatest_prime[i] ) {
            break;
        }
    }
    --i;
    if ( NULL != n2power ) {
        *n2power = ((uint64_t)1) << (MIN_POWER+i);
    }
    return (uint32_t)greatest_prime[i];
}

extern map_t *new_map( hash_fct hash, same_fct same,
                       uint32_t size, uint32_t collisions )
{
    map_t *map = malloc( sizeof(map_t) );
    if ( NULL == map ) {
        return NULL;
    }

    if ( 0 != size ) {
        if ( size < MIN_SIZE ) {
            size = MIN_SIZE;
        }
        uint64_t n2power;
        size = get_prime( size, &n2power ); // allocate just enough for prime modulo
        map_entry_t *table = malloc( sizeof(map_entry_t) * size );
        if ( NULL == table ) {
            free( map );
            return NULL;
        }
        memset( (void *)table, 0, sizeof(map_entry_t) * size );
        map->table = table;
        map->allocated = size;
        map->n2power = n2power;
    } else {
        map->table = NULL;
        map->allocated = 0;
        map->n2power = 0;
    }
    map->hash_f = hash;
    map->same_f = same;
    map->nb = 0;
    map->max_collision = 0;

    if ( collisions >= MIN_COLLISIONS ) {
        map->threshold = collisions;
    } else {
        map->threshold = MIN_COLLISIONS;
    }
    return map;
}

static void free_table( map_entry_t *table, uint32_t allocated )
{
    for ( uint32_t i = 0; i < allocated; ++i ) {
        map_entry_t *entry = &table[ i ];

        if ( entry->key ) { /* a valid  entry */
            map_entry_t *next;
            for ( entry = entry->next; entry != NULL; entry = next ) {
                next = entry->next;
                free( entry );
            }
        }
    }
    free( table );
}

extern int map_free( map_t *map )
{
    if ( NULL == map ) return -1;

    free_table( map->table, map->allocated );
    free( map );
    return 0;
}

// insert an entry in a collision list and return the number of entries
// in it after insertion, 0 if the entry could not be inserted, 1 if it
// is a new collision list (no collision yet) or a positive number greater
// than 1 in case of collisions.
static uint32_t add_entry( map_entry_t *entry, uint64_t hash,
                           const void *key, const void *data )
{
    assert( NULL != key );
    uint32_t count = 1;     // new entry will be added

    if ( entry->key ) {
        ++count;
        while ( entry->next ) {
            entry = entry->next;
            ++count;
        }

        map_entry_t *new_entry = malloc( sizeof(map_entry_t) );
        if ( NULL == new_entry ) {
            return 0;
        }
        entry->next = new_entry;
        entry = new_entry;
    }
    entry->next = NULL;
    entry->key  = key;
    entry->data = data;
    entry->hash = hash;
    return count;
}

static bool rehash( map_t *map, map_entry_t *new_table,
                    uint32_t new_allocated, uint64_t n2power )
{
  /* for each non-empty entry in the current table, apply the new allocated modulo
     and copy the entry into the new table */

    uint32_t nb = 0, max_chained = 0;
    for ( uint32_t i = 0; i < map->allocated ; ++i ) {
        map_entry_t *entry = &map->table[ i ];

        if ( entry->key ) { /* a valid  entry */
            while ( entry ) {
                uint32_t index = (uint32_t)(entry->hash % new_allocated);
                assert( index < new_allocated );
                uint32_t chained = add_entry( &new_table[ index ],
                                              entry->hash,
                                              entry->key, entry->data );
                if ( 0 == chained ) {
                    return false;   // keep old table as is
                }
                if ( max_chained < chained )
                    max_chained = chained;

                ++nb;
                entry = entry->next;
            }
        }
    }

    free_table( map->table, map->allocated );
    map->table = new_table;
    map->allocated = new_allocated;
    map->n2power = n2power;
    map->max_collision = max_chained - 1;
    map->nb = nb;
    return true;
}

// returns +1 if the table has been sucessfully re-allocated, -1 if it failed the
// re-allocation or rehashing and 0 if it did not have to re-allocate the table.
static int make_room( map_t *map )
{
    /* if less than 25% of entries left or more colliding entries than the
    // threshold in table, double the size */
    if ( ( 4 * (1 + map->nb) >= 3 * (map->allocated) ) ||
                            map->max_collision > map->threshold ) {
        /* starting from MIN_SIZE, double the size */
        if ( map->n2power >= (uint64_t)UINT32_MAX ) {
            return -1;          // no way to double it...
        }
        uint64_t n2power;
        uint32_t new_allocated = get_prime(
            ( map->n2power ) ?  2 * (map->n2power) : MIN_SIZE,
            &n2power );
        map_entry_t *new_table = malloc( sizeof(map_entry_t) * new_allocated );
        if ( NULL == new_table ) {
            return -1;          // no memory
        }

        memset( (void *)new_table, 0, sizeof(map_entry_t) * new_allocated );
        if ( map->table ) {
            if ( ! rehash ( map, new_table, new_allocated, n2power ) ) {
                free_table( new_table, new_allocated );
                return -1;      // failed to rehash
            }
        } else {
            map->table = new_table;
            map->allocated = new_allocated;
            map->n2power = n2power;
            map->max_collision = 0;
            map->nb = 0;
        }
        return 1;
    }
    return 0;
}

static inline uint64_t hash_data( const void *key )
{
    return (uint64_t)key;
}

static map_entry_t *get_entry( const map_t *map,
                               const void *key, uint64_t *hashp )
{
    if ( NULL == map || NULL == key ) {
        return NULL;
    }

    uint64_t hash = (NULL == map->hash_f) ? hash_data( key )
                                          : map->hash_f( key );
    if ( hashp ) {
        *hashp = hash;
    }

    if ( NULL == map->table ) {     // empty map
        return NULL;
    }

    uint32_t index = (uint32_t)(hash % map->allocated);
    map_entry_t *entry = &map->table[index];

    while( entry ) {
        if ( NULL != map->same_f ) {
            if ( map->same_f( entry->key, key ) )
                break;
        } else {
            if ( entry->key == key ) {
                break;
            }
        }
        entry = entry->next;
    }
    return entry;
}

extern const void *map_lookup_entry( const map_t *map, const void *key )
{
    map_entry_t *entry = get_entry( map, key, NULL );
    if ( NULL == entry ) {
        return NULL;
    }
    return entry->data;
}

extern const void *map_lookup_key( const map_t *map, const void *key )
{
    map_entry_t *entry = get_entry( map, key, NULL );
    if ( NULL == entry ) {
        return NULL;
    }
    return entry->key;
}

extern bool map_insert_entry( map_t *map, const void *key, const void *data )
{
    if ( NULL == key || NULL == map ) {         // needed as negative get_entry is expected
        return false;
    }
    uint64_t hash;
    map_entry_t *entry = get_entry( map, key, &hash );
    if ( NULL != entry ) {
        return false;                           // already in map
    }
    if ( -1 == make_room( map ) ) {             // extend if needed
        return false;                           // sorry, no memory
    }
    uint32_t index = hash % map->allocated;     // possibly new index
    uint32_t collisions = add_entry( &map->table[index], hash, key, data );
    if ( 0 == collisions ) {                    // unable to add entry
        return false;
    } else {
        --collisions;
    }
    if ( map->max_collision < collisions ) {
        map->max_collision = collisions;
    }
    ++map->nb;
    return true;
}

// does not attempt to shrink the hash table
extern bool map_delete_entry( map_t *map, const void *key )
{
    uint64_t hash;
    map_entry_t *entry = get_entry( map, key, &hash );
    if ( NULL == entry ) {
        return false;
    }

    uint32_t index = hash % map->allocated;
    map_entry_t *prev = NULL, *cur = &map->table[index];
    while ( cur != entry ) {
        prev = cur;
        cur = cur->next;
        assert( cur );
    }
    if ( prev ) {
        prev->next = entry->next;
        free( entry );
    } else {
        entry->key = entry->data = NULL;
        entry->hash = 0;
    }
    --map->nb;
    return true;
}

extern size_t map_len( const map_t *map )
{
    if ( NULL == map ) return 0;
    return map->nb;
}

extern slice_t *map_keys( const map_t *map, comp_fct cmp )
{
    if ( NULL == map ) return NULL;

    slice_t *slice = new_slice( sizeof( void *), map->nb );
    if ( NULL == slice ) return NULL;

    for ( uint32_t i = 0; i < map->allocated; ++i ) {
        map_entry_t *entry = &map->table[ i ];
        if ( entry->key ) {
            while ( entry ) {
                _pointer_slice_append_item( slice, entry->key );
                entry = entry->next;
            }
        }
    }
    if ( NULL != cmp ) {
        slice_sort_items( slice, cmp );
    }
    return slice;
}

extern void map_process_entries( const map_t *map, entry_process_fct proc,
                                 void *context )
{
    if ( NULL == map || NULL == proc ) return;

    for ( uint32_t i = 0; i < map->allocated; i ++ ) {
        map_entry_t *entry = &map->table[ i ];

        if ( entry->key ) { /* a valid  entry */
            while ( entry ) {
                if ( proc( i, entry->key, entry->data, context ) ) return;
                entry = entry->next;
            }
        }
    }
}

extern void map_stats( const map_t *map )
{
    printf( "Map %p stats\n", (void *)map );
    printf( "  entries          %d\n", map->nb );
    printf( "  allocated room   %d\n", map->allocated );
    printf( "  max collisions   %d\n\n", map->max_collision );

    printf( "  Entries {\n" );
    map_entry_t *entries = map->table;
    if ( entries ) {
        for( uint32_t i = 0; i < map->allocated; i++, entries++ ) {
            if ( entries->key ) {
                printf( "   Index %d, key 0x%zx, hash 0x%zx\n",
                        i, (size_t)(entries->key), entries->hash );
                for ( map_entry_t *collide = entries->next; collide;
                                              collide = collide->next ) {
                    printf( "   Index %d, key 0x%zx, hash 0x%zx\n",
                            i, (size_t)(collide->key), collide->hash );
                }
            }
        }
    }
    printf( "  }\n");
}
