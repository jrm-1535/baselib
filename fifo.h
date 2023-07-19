
#ifndef __FIFO_H__
#define __FIFO_H__

#include <stddef.h>
#include <stdbool.h>

#include "slice.h"
#include "node.h"

/*
    Fifo queue, only inserting at tail and extracting at head.

    The queue is a single linked list with tail pointer.

    Inserting multiple nodes can be done in one operation by giving a vector
    of nodes to insert. That vector is freed when all nodes in the vector have
    been extracted.

    In all cases nodes are supposed to be pointers to some objects in memory.
    When the fifo is deleted, all objects pointed to by items still in the
    queue are deleted. A free_node function can be given to replace the regular
    free function when the queue is deleted. This can be used to prevent
    freeing objects or to do additional cleanup before freeing a node.
*/

typedef struct _fifo fifo_t;

// create a new empty fifo queue. A node_free function can be given to replace
// the regular free (see node.h for free_node_fct definition). It the argument
// is NULL, nodes are deleted by calling free.
extern fifo_t *new_fifo( node_free_fct node_free );

// delete an existing fifo queue and its non-extracted nodes depending on the
// argument free_nodes.
extern void fifo_free( fifo_t * q , bool free_nodes );

// append a single item after the queue tail
extern int fifo_insert( fifo_t * q, void *node );

// append a slice of nodes after the queue tail. When returning from the call,
// fifo now manages the slice and its items (the slice will be freed when all
// its items have been extracted).
extern int fifo_insert_slice( fifo_t *q, slice_t *slice );

// extract the node at queue head. The returned data pointer must now be
// managed by the caller, as it won't be freed anymore.
extern void *fifo_extract( fifo_t * q );

// process all fifo nodes by calling the node_process_fct fct (see node.h for
// node_process_fct definition), unless the function returns true, in which
// case fifo_process_nodes stops immediately.
extern void fifo_process_nodes( const fifo_t *q, node_process_fct fct,
                                void * context );
#endif /* __FIFO_H__ */
