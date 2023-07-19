
#include <stdio.h>
#include <stdlib.h>

#include "slice.h"
#include "_slice.h"
#include "fifo.h"

typedef struct _slltentry {
    struct _slltentry * next;
    void *              data;   // either node or slice pointer
    bool                slice;
} slltentry;

struct _fifo {
    slltentry *     head;
    slltentry *     tail;

    node_free_fct   node_free;  // free function
    int             start;      // index inside vector at head
};

extern fifo_t *new_fifo( node_free_fct node_free )
{
    fifo_t *fifo = malloc( sizeof( fifo_t ) );

    if ( NULL != fifo ) {
        fifo->head = fifo->tail = NULL;
        if ( node_free ) {
            fifo->node_free = node_free;
        } else {
            fifo->node_free = free;
        }
        fifo->start = 0;
    }
    return fifo;
}

static inline void free_slice_items( slice_t *s, size_t from, node_free_fct f )
{
    size_t n = _slice_len( s );
    while ( n > from ) {
        --n;
        f( _pointer_slice_item_at( s, n ) );
    }
}

extern void fifo_free( fifo_t * q, bool free_nodes )
{
    slltentry *next = q->head;
    while ( true ) {
        slltentry *entry = next;
        if ( NULL == entry ) break;

        if ( free_nodes ) {
            if ( entry->slice ) {
                free_slice_items( (slice_t *)(entry->data),
                                   q->start, q->node_free );
            } else {
                q->node_free ( entry->data );
            }
        }
        if ( entry->slice ) {
            slice_free( entry->data );
            entry->slice = false;
        }
        next = entry->next;
        entry->next = NULL;
        entry->data = NULL;
        free( entry );
        q->start = 0;
    }
    q->head = q->tail = NULL;
    free( q );
}

extern int fifo_insert( fifo_t * q, void *data )
{
    if ( NULL == q || NULL == data ) return -1;

    slltentry *entry = malloc( sizeof( slltentry ) );
    if ( NULL == entry ) return -1;

    entry->next = NULL;
    entry->data = data;
    entry->slice = false;

    if ( NULL != q->tail ) {
        q->tail->next = entry;
    }
    q->tail = entry;
    if ( NULL == q->head ) {
        q->head = entry;
    }
    return 0;
}

extern int fifo_insert_slice( fifo_t *q, slice_t *slice )
{
    if ( NULL == q || NULL == slice ||
        sizeof(void *) != _slice_item_size( slice ) ) return -1;

    slltentry *entry = malloc( sizeof( slltentry ) );
    if ( NULL == entry ) return -1;

    entry->next = NULL;
    entry->data = slice;
    entry->slice = true;

    if ( NULL != q->tail ) {
        q->tail->next = entry;
    }
    q->tail = entry;
    if ( NULL == q->head ) {
        q->head = entry;
    }
    return 0;
}

static inline void sll_next( fifo_t *q, slltentry *head )
{
    q->head = head->next;
    head->next = NULL;
    head->data = NULL;

    if ( NULL == q->head ) {
        q->tail = NULL;
    }
    q->start = 0;
    free( head );
}

extern void *fifo_extract( fifo_t * q )
{
    if ( NULL == q || NULL == q->head ) return NULL;

    void *node;
    slltentry *head = q->head;

    if ( head->slice ) {
        int start = q->start;
        node = _pointer_slice_item_at( (slice_t *)head->data, start );

        q->start = start + 1;
        if ( q->start >= _slice_len( (slice_t *)head->data ) ) {
            slice_free( (slice_t *)head->data );
            sll_next( q, head );
        }

    } else {
        node = head->data;
        sll_next( q, head );
    }
    return node;
}

extern void fifo_process_nodes( const fifo_t *q, node_process_fct fct,
                                void * context )
{
    if ( NULL == q || NULL == fct ) return;

    size_t start = q->start;
    int i = 0;
    for ( slltentry *entry = q->head; entry != NULL; entry = entry->next ) {
        if ( entry->slice ) {
            for ( size_t index = start;
                    index < _slice_len( (slice_t *)entry->data ); ++index ) {
                if ( fct( i++,
                          _pointer_slice_item_at( (slice_t *)entry->data,
                                                   index ),
                          context ) ) {
                    return;
                }
            }
        } else {
            if ( fct( i++, entry->data, context ) ) {
                return;
            }
        }
        start = 0;
    }
}

