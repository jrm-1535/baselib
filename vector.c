
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "vector.h"
#include "_vector.h"

// The growing allocation numbers should be 2 powers
#define ARRAY_MINIMUM_ALLOCATION_NUMBER     8
// initially double previous size until threshold, then reduce increase
#define ARRAY_ALLOCATION_NUMBER_THRESHOLD   4096

static size_t get_2_power( size_t number )
{
    size_t allocation = ARRAY_MINIMUM_ALLOCATION_NUMBER;
    while ( number > allocation ) {
        if ( number < ARRAY_ALLOCATION_NUMBER_THRESHOLD ) {
            allocation *= 2;
        } else {
            allocation += ARRAY_ALLOCATION_NUMBER_THRESHOLD;
        }
    }
    return allocation;
}

extern vector_t *new_vector( size_t item_size, size_t number )
{
    void *data;
    if ( number > 0 ) {
        // Initial allocation is always as requested.
        size_t allocation = item_size * number;
        data = malloc( allocation );
        if ( NULL == data ) return NULL;
    } else {
        data = NULL;
    }

    vector_t *vector = malloc( sizeof(vector_t) );
    if ( NULL == vector ) {
        free( data );
        return NULL;
    }
    vector->data = data;
    vector->ref_count = 1;
    vector->item_size = item_size;
    vector->number = number;

    return vector;
}

extern vector_t *new_vector_from_data( const void *data,
                                       size_t item_size, size_t number )
{
    if ( NULL == data ) return NULL;

    vector_t *vector = new_vector( item_size, number );
    if ( NULL != vector ) {
        memcpy( vector->data, data, item_size * number );
    }
    return vector;
}

extern void vector_zero( vector_t *vector )
{
    if ( NULL == vector ) return;
    _vector_zero( vector );
}

extern void vector_segment_zero( vector_t * vector, size_t start, size_t len )
{
    if ( NULL == vector ) return;
    _vector_segment_zero( vector, start, len );
}

extern void * vector_item_at( const vector_t *vector, size_t index )
{
    if ( NULL == vector || index >= vector->number ) return 0;
    return _vector_item_at( vector, index );
}

extern char *vector_read_string( const vector_t *vector )
{
    return (char *)vector_item_at( vector, 0 );
}

extern void * pointer_vector_item_at( const vector_t *vector, size_t index )
{
    if ( NULL == vector || sizeof(void *) != vector->item_size ||
         index >= vector->number ) return 0;
    return _pointer_vector_item_at( vector, index );
}

extern int vector_write_item_at( vector_t *vector,
                                 size_t index, const void *data )
{
    if ( NULL == vector || NULL == data || index >= vector->number ) return -1;
    _vector_write_item_at( vector, index, data );
    return 0;
}

extern int pointer_vector_write_item_at( vector_t *vector,
                                         size_t index, const void *data )
{
    if ( NULL == vector || sizeof(void *) != vector->item_size ||
         index >= vector->number ) return -1;
    _pointer_vector_write_item_at( vector, index, data );
    return 0;
}

extern void vector_share( vector_t *vector )
{
    if ( NULL == vector ) return;

    ++vector->ref_count;
}

extern int vector_references( const vector_t *vector )
{
    if ( NULL == vector ) return -1;

    return vector->ref_count;
}

extern size_t vector_item_size( const vector_t * vector )
{
    if ( NULL == vector ) return 0;
    return _vector_item_size( vector );
}

extern size_t vector_cap( const vector_t *vector )
{
    if ( NULL == vector ) return 0;
    return _vector_cap( vector );
}

extern void vector_free( vector_t *vector )
{
    if ( NULL == vector ) return;

    if ( vector->ref_count > 1 ) {
        --vector->ref_count;
    } else if ( vector->ref_count == 1 ) {
        free( vector->data );
        vector->ref_count = 0;
        free( vector );
    }
}

extern vector_t *vector_grow( vector_t *vector )
{
    if ( NULL == vector ) return NULL;

    size_t number = get_2_power( vector->number + 1 );

    void *data;
    if ( vector->ref_count == 1 ) { // reuse the current vector with new data
        data = realloc( vector->data, vector->item_size * number );
        if ( NULL == data ) return NULL;
    } else {                        // make separate vector with new data
        data = malloc( vector->item_size * number );
        if ( NULL == data ) return NULL;

        --vector->ref_count;        // remove 1 reference from parent vector

        size_t item_size = vector->item_size;
        vector = malloc( sizeof(vector_t) );    // switch to new vector
        if ( NULL == vector ) {
            free( data );
            return NULL;
        }
        vector->ref_count = 1;
        vector->item_size = item_size;
    }
    vector->data = data;
    vector->number = number;

    return vector;
}

extern void vector_process_items( const vector_t *vector, item_process_fct fct,
                                  size_t start, size_t len, void * context )
{
    if ( NULL == vector || NULL == vector->data ) return;

    size_t number = vector_cap( vector );
    if ( start >= number ) return;

    if ( start + len > number ) {
         len = number - start;
    }

    size_t item_size = vector->item_size;
    uint8_t *ptr = ((uint8_t *)vector->data + start * item_size );

    for ( size_t i = 0; i < len; ++i ) {
        if ( fct( i, (void *)ptr, context ) ) break;
        ptr += item_size;
    }
}

extern void pointer_vector_process_items( const vector_t *vector,
                                          item_process_fct fct,
                                          size_t start, size_t len,
                                          void * context )
{
    if ( NULL == vector || sizeof(void *) != vector->item_size ||
         NULL == vector->data ) return;

    size_t number = vector_cap( vector );
    if ( start >= number ) return;

    if ( start + len > number ) {
         len = number - start;
    }

    size_t item_size = vector->item_size;
    uint8_t *ptr = ((uint8_t *)vector->data + start * sizeof(void *));

    for ( size_t i = 0; i < len; ++i ) {
        if ( fct( i, *(void **)ptr, context ) ) break;
        ptr += item_size;
    }
}
