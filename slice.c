
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "vector.h"
#include "slice.h"
#include "_slice.h"

extern slice_t *new_slice_with_vector( vector_t *vector, size_t len )
{
    if ( vector == NULL ) return NULL;

    slice_t *slice = malloc( sizeof( slice_t ) );
    if ( slice ) {
        slice->vector = vector;
        slice->start = 0;
        slice->len = len;
    }
    return slice;
}

extern slice_t *new_slice( size_t item_size, size_t number )
{
    vector_t *vector = new_vector( item_size, number );
    return new_slice_with_vector( vector, 0 );
}

extern slice_t *new_slice_from_data( const void *data,
                                     size_t item_size, size_t number )
{
    vector_t *vector = new_vector_from_data( data, item_size, number );
    return new_slice_with_vector( vector, number );
}

// start and beyond are relative to the existing slice. They must both be
// between 0 and the existing slice length, and start must be <= beyond.
extern slice_t *new_slice_from_slice( const slice_t *slice,
                                      size_t start, size_t beyond )
{
    if ( NULL == slice || start > beyond ||
        beyond > _slice_len( slice ) ) return NULL;

    slice_t *new_slice = malloc( sizeof( slice_t ) );
    if ( NULL != new_slice ) {
        vector_share( slice->vector );
        new_slice->vector = slice->vector;
        new_slice->start = slice->start + start;
        new_slice->len = beyond - start;
    }
    return new_slice;
}

extern slice_t *slice_dup( const slice_t * slice )
{
    if ( NULL == slice ) {
        return NULL;
    }
    return new_slice_from_slice( slice, 0, slice->len );
}


// set all items in the slice [0..len] to 0.
extern void slice_zero( slice_t *slice ) {
    if ( NULL != slice ) {
        vector_segment_zero( slice->vector, slice->start, slice->len );
    }
}

extern void *slice_set_use( slice_t *slice, void *use )
{
    if ( NULL == slice ) {
        return NULL;
    }
    return _slice_set_use( slice, use );
}

extern void *slice_get_use( slice_t *slice )
{
    if ( NULL == slice ) {
        return NULL;
    }
    return _slice_get_use( slice );
}

// update the slice length, as long as it fits in [0..cap]
extern int slice_update_len( slice_t *slice, size_t len )
{
    if ( NULL == slice || len > _slice_cap( slice ) )
        return -1;

    _slice_update_len( slice, len );
    return 0;
}

extern size_t slice_cap( const slice_t *slice )
{
    if ( NULL == slice ) return 0;
    return _slice_cap( slice );
}

extern size_t slice_len( const slice_t *slice )
{
    if ( NULL == slice ) return 0;
    return _slice_len( slice );
}

// access to the underlying data
extern void *slice_data_n_len( const slice_t *slice, size_t *lenp )
{
    if ( NULL == slice )  return NULL;
    return _slice_data_n_len( slice, lenp );
}

extern size_t slice_item_size( const slice_t * slice )
{
    if ( NULL == slice ) return 0;
    return _vector_item_size( slice->vector );
}

extern void * slice_item_at( const slice_t *slice, size_t index )
{
    if ( NULL == slice || index >= slice->len ) return 0;
    return _slice_item_at( slice, index );
}

extern void * pointer_slice_item_at( const slice_t *slice, size_t index )
{
    if ( NULL == slice || _vector_item_size(slice->vector) != sizeof(void *) ||
         index >= slice->len ) return 0;
    return _pointer_slice_item_at( slice, index );
}

extern int slice_write_item_at( slice_t *slice,
                                size_t index, const void * data )
{
    if ( NULL == slice || NULL == data || index >= slice->len ) return -1;
    _slice_write_item_at( slice, index, data );
    return 0;
}

extern int pointer_slice_write_item_at( slice_t *slice,
                                        size_t index, const void * data )
{
    if ( NULL == slice || _vector_item_size(slice->vector) != sizeof(void *) ||
         index >= slice->len ) return -1;
    _pointer_slice_write_item_at( slice, index, data );
    return 0;
}

static inline int _slice_insert_hole( slice_t *slice, size_t index )
{
    if ( NULL == slice || index > slice->len ) return -1;
    if ( ! _slice_make_room( slice ) ) return -1;

    if ( index < slice->len ) {
        if ( -1 == vector_move_items( slice->vector,
                slice->start + index, slice->start + slice->len-1, 1 ) ) {
            return -1;
        }
    }
    ++slice->len;
    return 0;
}

extern int slice_insert_item_at( slice_t *slice, size_t index,
                                 const void *data )
{
    if ( NULL == data || -1 == _slice_insert_hole( slice, index ) ) {
        return -1;
    }
    _slice_write_item_at( slice, index, data );
    return 0;
}

extern int pointer_slice_insert_item_at( slice_t *slice, size_t index,
                                         const void *data )
{
    if ( NULL == data || -1 == _slice_insert_hole( slice, index ) ) {
        return -1;
    }
    _pointer_slice_write_item_at( slice, index, data );
    return 0;
}

extern int slice_remove_item_at( slice_t *slice, size_t index )
{
    if ( NULL == slice || index >= slice->len ) return -1;
    if ( -1 == vector_move_items( slice->vector,
                    slice->start + index +1, slice->start + slice->len, -1 ) ) {
        return -1;
    }
    --slice->len;
    return 0;
}

extern int slice_move_items_from( slice_t *slice,
                                  size_t index, size_t len, ssize_t offset )
{
    if ( NULL == slice || index + len > slice->len ||
        (ssize_t)index + offset < 0 ||
        (ssize_t)index + (ssize_t)len + offset > (ssize_t)slice->len ) {
        return -1;
    }
    if ( -1 == vector_move_items( slice->vector,
                                  slice->start + index,
                                  slice->start + index + len-1, offset ) ) {
        return -1;
    }
    return 0;
}

static inline void _swap_uint8( void *start, size_t offset1, size_t offset2 )
{
    uint8_t *item1 = (uint8_t *)start + offset1;
    uint8_t *item2 = (uint8_t *)start + offset2;

    uint8_t tmp = *item1;
    *item1 = *item2;
    *item2 = tmp;
}

static inline void _swap_uint16( void *start, size_t offset1, size_t offset2 )
{
    uint16_t *item1 = (uint16_t *)((uint8_t *)start + offset1);
    uint16_t *item2 = (uint16_t *)((uint8_t *)start + offset2);

    uint16_t tmp = *item1;
    *item1 = *item2;
    *item2 = tmp;
}

static inline void _swap_uint32( void *start, size_t offset1, size_t offset2 )
{
    uint32_t *item1 = (uint32_t *)((uint8_t *)start + offset1);
    uint32_t *item2 = (uint32_t *)((uint8_t *)start + offset2);

    uint32_t tmp = *item1;
    *item1 = *item2;
    *item2 = tmp;
}

static inline void _swap_uint64( void *start,
                                size_t offset1, size_t offset2, size_t n )
{
    uint64_t *item1 = (uint64_t *)((uint8_t *)start + offset1);
    uint64_t *item2 = (uint64_t *)((uint8_t *)start + offset2);

    for ( int i = (int)n; i; ) {
        --i;
        uint64_t tmp = item1[i];
        item1[i] = item2[i];
        item2[i] = tmp;
    }
}

extern int slice_swap_items( slice_t *slice, size_t index1, size_t index2 )
{
    if ( NULL == slice || index1 == index2 ||
         index1 >= slice->len || index2 >= slice->len )
        return -1;

    size_t item_size = _vector_item_size( slice->vector );
    void *start = _vector_index_ptr_at( slice->vector, slice->start );
    size_t offset1 = index1 * item_size;
    size_t offset2 = index2 * item_size;

    if ( item_size >= 8 ) {
        _swap_uint64( start, offset1, offset2, item_size / 8 );
        size_t remaining = item_size % 8;
        offset1 += item_size - remaining;
        offset2 += item_size - remaining;
        item_size = remaining;
    } // here 0 <= item_size < 8
    if ( item_size >= 4 ) { // at most 1 32-bit swap
        _swap_uint32( start, offset1, offset2 );
        offset1 += 4;
        offset2 += 4;
        item_size = item_size - 4;
    } // here 0 <= item_size < 4
    if ( item_size >= 2 ) { // at most 1 16-bit swap
        _swap_uint32( start, offset1, offset2 );
        offset1 += 2;
        offset2 += 2;
        item_size = item_size - 2;
    } // here 0 <= item_size < 2
    if ( item_size > 0 ) {  // at most 1 8-bit swap
        _swap_uint8( start, offset1, offset2 );
    }
    return 0;
}

extern int slice_append_item( slice_t *slice, const void * data )
{
    if ( NULL == slice || NULL == data ) return -1;
    return _slice_append_item( slice, data );
}

extern int pointer_slice_append_item( slice_t *slice, const void * data )
{
    if ( NULL == slice ) return -1;
    return _pointer_slice_append_item( slice, data );
}


extern int slice_free( slice_t *slice )
{
    if ( NULL == slice ) return -1;

    vector_free( slice->vector );
    free( slice );
    return 0;
}

extern void slice_process_items( const slice_t *slice, item_process_fct fct,
                                 void *context )
{
    if ( NULL == slice || NULL == fct ) return;
    vector_process_items( slice->vector, fct, slice->start, slice->len,
                          context );
}

extern int pointer_slice_finalize( slice_t *slice, void (*f)( void *) )
{
    if ( NULL == slice ) return -1;

    size_t n = slice_len( slice );
    for ( size_t i = 0; i < n; ++i ) {
        f( pointer_slice_item_at( slice, i ) );
        pointer_slice_write_item_at( slice, i, NULL ); // in case of shared vector
    }
    return slice_free( slice );
}

extern int pointer_slice_free( slice_t *slice )
{
/*
    if ( NULL == slice ) return -1;

    size_t n = slice_len( slice );
    for ( size_t i = 0; i < n; ++i ) {
        free( pointer_slice_item_at( slice, i ) );
        pointer_slice_write_item_at( slice, i, NULL ); // in case of shared vector
    }
    return slice_free( slice );
*/
    return pointer_slice_finalize( slice, free );
}

extern bool slice_sort_items( slice_t *slice, comp_fct cmp )
{
    if ( NULL == slice || NULL == cmp ) {
        return false;
    }
    qsort( _slice_item_at( slice, 0 ), _slice_len( slice ),
           _vector_item_size( slice->vector ),  cmp );
    return true;
}

extern void pointer_slice_process_items( const slice_t *slice,
                                         item_process_fct fct,
                                         void *context )
{
    pointer_vector_process_items( slice->vector, fct,
                                  slice->start, slice->len, context );
}
