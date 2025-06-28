#include <stddef.h>
#include <stdbool.h>

#include "slice.h"
/*
    Binary heap is a partially sorted data structure using a dynamic array or
    slice as a basis (array that grows automatically when it gets full, and
    provides amortized access time in O(1)).

    This implementation takes arbitrary data types as heap elements, which
    requires that a comparison function be provided for sorting elements.

    The comparison function returns the relative value, or priority, of two
    elements in the heap. If the function returns value( 1 ) - value( 2 ), then
    the binary heap is such that a parent element has always a larger value than
    any of its children (max heap). The root of the binary heap, at position 0
    in the array, is then the item with the largest value (or priority).

    If on the contrary the comparison function returns value( 2 ) - value( 1 ),
    the binary heap has the smaller value at its root (min heap).

    A binary heap can be used to implement heap sort or to implement a priority
    queue.

    Heap operations (see below for detailed explanations) are:

            operation               time complexity
        new empty heap                  O(1)
        new from data                   O(n)
        new from slice                  O(n)
        insert                          O(log n)
        peek                            O(1)
        extract                         O(log n)
        insert_then_extract             O(log n)
        extract_then_insert             O(log n)
        traverse heap                   O(n)

    In this implemetation a heap always contains pointers to objects in memory
    (void *). Freeing a heap does not automativally free the objects pointed to
    by items still in it.

    All operations are in-place, assuming the heap slice automatically grows
    as needed. The traverse heap operation is in O(n) worst case since it can
    be stopped as soon as the entry to search is found (traversing starts
    always at root).
*/

typedef struct heap heap_t;

// cmp_fct needs just to return val(item1) - val(item2)
typedef int (*cmp_fct)( void *item1, void *item2 );

// create a new empty heap for an initial number of entries. Internally
// a heap uses a slice that can grow as needed.
extern heap_t *new_heap( size_t number, cmp_fct cmp );

// create a hew heap for initially number (void *)elements and store the
// comparison function to call while sorting. The data passed is a pointer
// to an array of object pointers, which are copied into the heap before it
// is heapified. It returns the heap or a pointer if memory allocation fails.
extern heap_t *new_heap_from_data( const void **data,
                                   size_t number, cmp_fct cmp );

// create a new heap from the given pointer slice. Slice elements must be
// pointers.The comparison function is used to compare those elements.
// The slice is used for storage and items are heapiffied in place.
extern heap_t *new_heap_from_pointer_slice( slice_t *slice, cmp_fct cmp );

// free an existing heap, without freeing any object still pointed to by
// elements in the heap.
extern void heap_free( heap_t *heap );

// heap_len returns the number of items in the heap
extern size_t heap_len( heap_t *heap );

// append an element and rebalance the heap. The argument data is a pointer to
// the object in memory. It returns true in case of success, false otherwise.
extern bool heap_insert( heap_t *heap, void *data );

// If the heap is empty (root does not exist) heap_peek returns NULL, otherwise
// it returns a pointer to the object currently pointed to by heap root.
extern void * heap_peek( heap_t *heap );

// extract the heap root element and rebalance the heap. If the heap is empty
// it just returns NULL, otherwise it removes the heap root, selects the new
// heap root and returns a pointer to the object that was previously pointed
// to by heap root.
extern void *heap_extract( heap_t *heap );

// insert new element, rebalance the heap, then extract root. If the heap was
// empty, it is left empty and the new element is returned as root. It returns
// the extracted element in case of success, or NULL otherwise.
extern void *heap_insert_then_extract( heap_t *heap, void *data );

// extract the heap root, then insert a new element and rebalance the heap. If
// the heap was empty the new element is inserted but the return value is NULL.
extern void *heap_extract_then_insert( heap_t *heap, void *data );

// replace an element in heap and rebalance the heap. If the element to replace
// is not in the heapm it returns false, otherwise true.
extern bool heap_replace_at( heap_t *heap, size_t item, void *data );

// same as replace, but the extra argument up indicates whether the rebalancing
// is required only toward the head of the heap (if true) or toward the tail of
// the heap (if false). This assumes that the caller knows whether it is a min
//-heap or a max-heap, and if the new value is greater or smaller than the
// current one. This extra argument saves one call to the heap compare function.
extern bool heap_update_at( heap_t *heap, size_t item, void *data, bool up );

// heap_process_items calls the item_process_function function for each item in
// heap until it returns true or the end of the heap has been reached. The
// context data are passed as is to the function (for its definition see
// vector.h). The traversal order is the flat heap array, from root.
extern void heap_process_items( const heap_t *heap, item_process_fct fct,
                                void *context );

// heap_check returns true if the heap is valid, false otherwise
extern bool heap_check( const heap_t *heap );
