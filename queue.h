
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include <stdbool.h>

#include "node.h"

/*
    Simple queue for fifo, lifo or mixed usage based on a double linked list
    with tail.
*/

typedef struct _queue queue_t;

// create a hew empty queue.A free_node function can be given to replace
// the regular free (see node.h for free_node_fct definition). It the argument
// is NULL, nodes are deleted by calling free.
extern queue_t *new_queue( node_free_fct free_node );

// delete a queue and free all nodes still in queue depending on the argument
// free_nodes.
extern void queue_free( queue_t *q, bool free_nodes );

// insert one item before queue head
extern int queue_head_insert( queue_t *q, void *node );

// append one node after queue tail
extern int queue_tail_append( queue_t *q, void *node );

// return the queue length
extern size_t queue_length( const queue_t *q );

// peek at the head of queue (no effect on queue). The data pointed to by the
// returned pointer should not be modified...
extern void *queue_head_peek( const queue_t *q );

// peek at the tail of queue (no effect on queue). The data pointed to by the
// returned pointer should not be modified...
extern void *queue_tail_peek( const queue_t *q );

// extract item at queue head
extern void *queue_head_extract( queue_t *q );

// extract node at queue tail
extern void *queue_tail_extract( queue_t *q );

// process all queue nodes by calling the node_process_fct fct (see node.h for
// node_process_fct definition), unless fct returns true, in which case
// queue_process_nodes stops immediately.
extern void queue_process_nodes( const queue_t *q, node_process_fct fct,
                                 void *context );

#endif /* __QUEUE_H__ */
