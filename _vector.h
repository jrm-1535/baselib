
#ifndef __IVECTOR_H__
#define __IVECTOR_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vector.h"

// fast inline version without argument checking

struct vector {
    void    *data;
    size_t  ref_count;
    size_t  item_size;
    size_t  number;
};

static inline void _vector_zero( vector_t *vector )
{
    memset( vector->data, 0, vector->item_size * vector->number );
}

static inline void _vector_segment_zero( vector_t * vector,
                                         size_t start, size_t len )
{
    if ( start + len <= vector->number ) {
        memset( (void *)((char *)vector->data + vector->item_size * start), 0,
                 vector->item_size * len );
    }
}

static inline uint8_t * _vector_index_ptr_at( const vector_t *vector,
                                              size_t index )
{
    return ((uint8_t *)vector->data + index * vector->item_size );
}

static inline void * _vector_item_at( const vector_t *vector, size_t index )
{
    return (void *)_vector_index_ptr_at( vector, index );
}

static inline void * _pointer_vector_item_at( const vector_t *vector,
                                              size_t index )
{
    return *(void **)_vector_index_ptr_at( vector, index );
}

static inline void _vector_write_item_at( vector_t *vector,
                                          size_t index, const void *data )
{
    memcpy( (void *)_vector_index_ptr_at( vector, index ),
            data, vector->item_size );
}

static inline void _pointer_vector_write_item_at( vector_t *vector,
                                                  size_t index,
                                                  const void *data )
{
    *(void **)_vector_index_ptr_at( vector, index ) = (void *)data;
}


static inline size_t _vector_item_size( const vector_t * vector )
{
    return vector->item_size;
}

static inline size_t _vector_cap( const vector_t *vector )
{
    return vector->number;
}

#endif /* __IVECTOR_H__ */
