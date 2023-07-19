
#ifndef __NODE_H__
#define __NODE_H__

// user defined function called to free each node still in fifo when fifo_free
// is called. This can be used to prevent freeing objects or to do additional
// cleanup before freeing a node.
typedef void (*node_free_fct)( void *node );

// function called by queue_process_node or fifo_process_node for each node
// in queue or fifo respectively, unless the function returns true, in which
// case those functions stop immediately. The index argument is the rank in
// queue or fifo, starting from the head and node is the corresponding node
// pointer. The argument context comes the caller of queue_process_node or
// fifo_process_node.
typedef bool (*node_process_fct)( size_t index, void *node, void *context );

#endif /* __NODE_H__ */
