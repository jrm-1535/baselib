
#include <stdio.h>
#include <malloc.h>
#include <assert.h>

#include "queue.h"

struct _queue {
    struct _dllentry    *head;
    struct _dllentry    *tail;
    node_free_fct       node_free;
    size_t              size;
};

typedef struct _dllentry {
    struct _dllentry    *next;
    struct _dllentry    *prev;
    void                *node;
} dllentry;

extern queue_t *new_queue( node_free_fct node_free )
{
    queue_t *q = malloc( sizeof( queue_t ) );
    q->head = q->tail = NULL;
    q->size = 0;
    if ( node_free ) {
        q->node_free = node_free;
    } else {
        q->node_free = free;
    }
    return q;
}

extern void queue_free( queue_t *q, bool free_nodes )
{
    if ( NULL == q ) return;

    dllentry *next = NULL;
    for ( dllentry *entry = q->head; entry != NULL; entry = next ) {
        if ( free_nodes ) {
            q->node_free( entry->node );
        }
        next = entry->next;
        entry->next = entry->prev = entry->node = NULL;
        free( entry );
    }
    q->head = q->tail = NULL;
    free( q );
}

extern int queue_head_insert( queue_t *q, void *node )
{
    if ( NULL == q || NULL == node ) return -1;

    dllentry *entry = malloc( sizeof( dllentry ) );
    if ( NULL == entry ) return -1;

    entry->node = node;
    entry->prev = NULL;
    entry->next = q->head;
    if ( q->head ) {
        q->head->prev = entry;
    }
    q->head = entry;
    if ( NULL == q->tail ) {
        q->tail = entry;
    }

    ++ q->size;
    return 0;
}

extern int queue_tail_append( queue_t *q, void *node )
{
    if ( NULL == q || NULL == node ) return -1;

    dllentry *entry = malloc( sizeof( dllentry ) );
    if ( NULL == entry ) return -1;

    entry->node = node;
    entry->next = NULL;
    entry->prev = q->tail;
    if ( q->tail ) {
        q->tail->next = entry;
    }
    q->tail = entry;
    if ( NULL == q->head ) {
        q->head = entry;
    }

    ++ q->size;
    return 0;
}

extern size_t queue_length( const queue_t *q )
{
    if ( NULL == q ) return 0;
    return q->size;
}

extern void *queue_head_peek( const queue_t *q )
{
    if ( NULL == q || NULL == q->head ) return NULL;
    return q->head->node;
}

extern void *queue_tail_peek( const queue_t *q )
{
    if ( NULL == q || NULL == q->tail ) return NULL;
    return q->tail->node;
}

extern void *queue_head_extract( queue_t *q )
{
    if ( NULL == q || NULL == q->head ) return NULL;

    dllentry *head = q->head;
    q->head = head->next;
    if ( NULL != q->head ) {
        q->head->prev = NULL;
    } else {
        q->tail = NULL;
    }
    void *node = head->node;
    free( head );

    --q->size;
    return node;
}

extern void *queue_tail_extract( queue_t *q )
{
    if ( NULL == q || NULL == q->tail ) return NULL;

    dllentry *tail = q->tail;
    q->tail = tail->prev;
    if ( NULL != q->tail ) {
        q->tail->next = NULL;
    } else {
        q->head = NULL;
    }
    void *node = tail->node;
    free( tail );

    --q->size;
    return node;
}

extern void queue_process_nodes( const queue_t *q, node_process_fct fct,
                                 void *context )
{
    if ( NULL == q || NULL == fct ) return;

    int i = 0;
    for ( dllentry *entry = q->head; entry != NULL; entry = entry->next ) {
        fct( i, entry->node, context );
        ++i;
    }
}

