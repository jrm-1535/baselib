

#include "fnv.h"


static inline uint64_t _fnv1a_64_append( uint64_t hash,
                                         uint8_t *data, size_t len )
{
    uint8_t *end = data + len;

    while ( data < end ) {
        hash ^= (uint64_t)(*data++);
#if FAST_MUL
        hash *= 0x00000100000001B3;     // 1099511628211;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 5) +
                (hash << 7) + (hash << 8) + (hash << 40);
#endif
    }
    return hash;
}

uint64_t fnv1a_64_append( uint64_t hash, uint8_t *data, size_t len )
{
    return _fnv1a_64_append( hash, data, len );
}

uint64_t fnv1a_64( uint8_t *data, size_t len )
{
    uint64_t hash = 0xcbf29ce484222325; // 14695981039346656037
    return _fnv1a_64_append( hash, data, len );
}
