
#ifndef __ISLICE_H__
#define __ISLICE_H__

#include <stddef.h>
#include "_vector.h"

// fast inline version without argument checking

struct slice {
    vector_t    *vector;
    void        *use;       // allow checking actual types and/or private data
    size_t      start;
    size_t      len;
};

static inline void *_slice_set_use( slice_t *slice, void * use )
{
    void *prev = slice->use;
    slice->use = use;
    return prev;
}

static inline void *_slice_get_use( slice_t *slice )
{
    return slice->use;
}

static inline void _slice_update_len( slice_t *slice, size_t len )
{
    slice->len = len;
}

static inline size_t _slice_cap( const slice_t *slice )
{
    return _vector_cap( slice->vector ) - slice->start;
}

static inline size_t _slice_len( const slice_t *slice )
{
    return slice->len;
}

static inline uint8_t * _slice_data_n_len( const slice_t *slice, size_t *lenp )
{
    if ( lenp ) { *lenp = slice->len; }
    return _vector_index_ptr_at( slice->vector, slice->start );
}

static inline size_t _slice_item_size( const slice_t * slice )
{
    return _vector_item_size( slice->vector );
}

static inline void * _slice_item_at( const slice_t *slice, size_t index )
{
    return _vector_item_at( slice->vector, slice->start + index );
}

static inline void * _pointer_slice_item_at( const slice_t *slice, size_t index )
{
    return _pointer_vector_item_at( slice->vector, slice->start + index );
}

static inline void _slice_write_item_at( slice_t *slice, size_t index,
                                         const void * data )
{
    _vector_write_item_at( slice->vector, slice->start + index, data );
}

static inline void _pointer_slice_write_item_at( slice_t *slice, size_t index,
                                                 const void * data )
{
    _pointer_vector_write_item_at( slice->vector, slice->start + index, data );
}

static inline bool _slice_make_room( slice_t *slice )
{
    if ( slice->len >= _slice_cap( slice ) ) {
        vector_t *vector = vector_grow( slice->vector );
        if ( NULL == vector ) {
            return false;
        }
        slice->vector = vector;
    }
    return true;
}

static inline int _slice_append_item( slice_t *slice, const void * data )
{
    if ( ! _slice_make_room( slice ) ) return -1;

    size_t index = slice->len++;
    _vector_write_item_at( slice->vector, slice->start + index, data );
    return 0;
}

static inline int _pointer_slice_append_item( slice_t *slice,
                                              const void * data )
{
    if ( ! _slice_make_room( slice ) ) return -1;

    size_t index = slice->len++;
    _pointer_vector_write_item_at( slice->vector, slice->start + index, data );
    return 0;
}
#endif /* __ISLICE_H__ */
