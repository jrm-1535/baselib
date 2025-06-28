
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "heap.h"
#include "slice.h"
#include "_slice.h"

struct heap {
    slice_t *slice;
    cmp_fct cmp;
};

/* percolate_up assumes the heap property was estabished before a new item was
   appended to the heap, possibly breaking the heap property.

   percolate_up: start from the last element and move up the heap until
                 the heap property is true (parent value is higher or
                 equal - or lower or equal, depending on the comparison
                 function - than both children). This assumes that the
                 heap is already valid and a new unknown value is appended,
                 which might need to percolate up the heap to recreate the
                 heap property.

    Worst case percolate_up loops down the complete tree height
                   O(1) for each swap, times Log2(n)
*/

static inline void percolate_up( heap_t *heap, size_t from )
{
    size_t n = _slice_len( heap->slice );
    if ( from >= n || from < 1 ) return;    // nothing to percolate up

    while ( from ) {
        int parent, bigger_child;
        if ( 0 == (from & 1) ) {            // even index = right child
            parent = from/2 - 1;

            int res = heap->cmp( _pointer_slice_item_at( heap->slice, from ),
                                 _pointer_slice_item_at( heap->slice, from-1 ) );
            if ( res > 0 ) {
                bigger_child = from;
            } else {
                bigger_child = from-1;
            }
        } else {                            // odd index = left child alone
            parent = (from-1)/2;
            bigger_child = from;
        }
        int res = heap->cmp( _pointer_slice_item_at( heap->slice, parent ),
                             _pointer_slice_item_at( heap->slice, bigger_child ) );
        if ( res >= 0 ) return;

        slice_swap_items( heap->slice, parent, bigger_child );
        from = parent;
    }
}

// append an element and rebalance the heap
extern bool heap_insert( heap_t *heap, void *data )
{
    if ( NULL != heap ) {
        if ( 0 == _pointer_slice_append_item( heap->slice, data ) ) {
            percolate_up( heap, _slice_len( heap->slice ) - 1 );
            return true;
        }
    }
    return false;
}

/* percolate_down assumes the heap property was established before the root
   was modified, and therefore may not be true for the root anymore.

   percolate_down:  start from the root and move down the heap until
                    the heap property is true (parent value is higher or
                    equal - or lower or equal, depending on the comparison
                    function - than both children). This assumes that the
                    heap is already valid and a new root value is set, which
                    might need to percolate down the heap to recreate the
                    heap property.

    Worst case percolate_down loops down the complete tree height
                   O(1) for each swap, times Log2(n)
*/
static void percolate_down( heap_t *heap, size_t from )
{
    size_t n = _slice_len( heap->slice );
    if ( from >= n/2 ) return;      // no possible children

    size_t bigger, left, right;     // bigger is a misnomer (only if max heap)

    while ( 1 ) {
        left = 2 * from + 1;        // find left child position
        if ( left >= n ) return;    // reached the end of the heap

        right = left + 1;
        if ( right >= n ) {         // no right child,
            bigger = left;          // left is necessarily the bigger
        } else {
            int res = heap->cmp( _pointer_slice_item_at( heap->slice, left ),
                                 _pointer_slice_item_at( heap->slice, right ) );
            if ( res < 0 ) {
                bigger = right;     // select bigger child, since it might
            } else {                // be bigger than its parent and require
                bigger = left;      // a swap to restore the heap property.
            }
        }

        int res = heap->cmp( _pointer_slice_item_at( heap->slice, from ),
                             _pointer_slice_item_at( heap->slice, bigger ) );
        if ( res >= 0 ) return;     // no need to swap, we are done

        slice_swap_items( heap->slice, from, bigger );
        from = bigger;              // keep moving down
    }
}

static inline void updade_item( heap_t *heap, size_t item,
                                void * data, bool inc )
{
    // replace item value
    _pointer_slice_write_item_at( heap->slice, item, data );

    if ( inc ) {
        percolate_up( heap, item );
    } else {
        percolate_down( heap, item );
    }
}

extern bool heap_replace_at( heap_t *heap, size_t item, void * data )
{
    if ( NULL == heap ) return false;

    size_t n = slice_len( heap->slice );
    if ( item >= n ) return false;

    bool inc;
    if ( heap->cmp( _pointer_slice_item_at( heap->slice, item ), data ) < 0 ) {
        inc = true;
    } else {
        inc = false;
    }

    updade_item( heap, item, data, inc );
    return true;
}

extern bool heap_update_at( heap_t *heap, size_t item, void * data, bool inc )
{
    if ( NULL == heap ) return false;

    size_t n = slice_len( heap->slice );
    if ( item >= n ) return false;

    updade_item( heap, item, data, inc );
    return true;
}

// peek the heap root
extern void * heap_peek( heap_t *heap )
{
    if ( NULL == heap || 0 == slice_len( heap->slice ) ) {
        return NULL;
    }
    return _pointer_slice_item_at( heap->slice, 0 );
}

// extract the root and rebalance the heap
extern void * heap_extract( heap_t *heap )
{
    if ( NULL == heap ) return NULL;

    size_t l = _slice_len( heap->slice );
    if ( 0 == l ) return false;

    // get current root data
    void *root_data = _pointer_slice_item_at( heap->slice, 0 );

    if ( 1 == l ) {     // root was the last element, just return root data
        _slice_update_len( heap->slice, 0 );
        return root_data;
    }

    // set last element in root
    _slice_write_item_at( heap->slice, 0, slice_item_at( heap->slice, l - 1 ) );

    // remove last element
    _slice_update_len( heap->slice, l - 1 );

    // rebalance and return true
    percolate_down( heap, 0 );
    return root_data;
}

extern void * heap_insert_then_extract( heap_t *heap, void *data )
{
    if ( NULL == heap ) return NULL;   // invalid call

    size_t l = _slice_len( heap->slice );
    if ( 0 == l ) {                 // if empty heap, return new data
        return data;
    }

    // else get current root data
    void *root_data = _pointer_slice_item_at( heap->slice, 0 );
    int cmp_res = heap->cmp( data, root_data );
    if ( cmp_res > 0 ) {    // new data is greater than root:
        return data;        // leave heap the same and return new data
    }
    // else val( data ) <= val( root_data ), set new data in root
    _pointer_slice_write_item_at( heap->slice, 0, data );

    percolate_down( heap, 0 );      // rebalance

    return root_data;
}

extern void *heap_extract_then_insert( heap_t *heap, void* data )
{
    if ( NULL == heap ) return NULL;       // invalid call

    size_t l = _slice_len( heap->slice );
    if ( 0 == l ) {         // if empty heap, just append data and return NULL
        _pointer_slice_append_item( heap->slice, data );
        return NULL;
    }

    // else get current root data
    void *root_data = _pointer_slice_item_at( heap->slice, 0 );

    // set new data in root
    _pointer_slice_write_item_at( heap->slice, 0, data );

    percolate_down( heap, 0 );  // rebalance

    return root_data;
}

extern heap_t *new_heap_from_slice( slice_t *slice, cmp_fct cmp )
{
    if ( NULL == slice || NULL == cmp ) {
        return NULL;
    }

    heap_t *heap = malloc( sizeof( heap_t ) );
    if ( NULL != heap ) {
        heap->slice = slice;
        heap->cmp = cmp;
        size_t n = slice_len( slice );
        if ( n > 1 ) {
            size_t i = (n-1)/2;             // start from the bottom of the heap
            do {                            // calls (n-1)/2 percolate_down(n)
                percolate_down( heap, i );  // to establish the heap property
            } while ( i-- );                // from the bottom up
        }
    }
    return heap;
}

extern heap_t *new_heap( size_t number, cmp_fct cmp )
{
    if ( NULL == cmp ) return NULL;

    heap_t *heap = NULL;
    slice_t *slice = new_slice( sizeof( void *), number );
    if ( NULL != slice ) {
        heap = new_heap_from_slice( slice, cmp );
        if ( NULL == heap ) {
            slice_free( slice );
        }
    }
    return heap;
}

/* (n-1)/2 calls to percolate_down, but bottom calls are in O(1):
   (n/4 * O(1)) + (n/8 * 2 * O(1)) + ... + ( n/2^i * i * O(1)
   n * (1/4 + 2/8 + 3/16 + ... + i/2^(i+1))
   Since Sum [i=1 -> infinite] (i/2^(i+1)) is 1, time complexity is O(n)
*/
extern heap_t *new_heap_from_data( const void **data,
                                   size_t number, cmp_fct cmp )
{
    if ( NULL == data || NULL == cmp ) return NULL;

    heap_t *heap = NULL;
    slice_t *slice = new_slice_from_data( data, sizeof( void *), number );
    if ( NULL != slice ) {
        heap = new_heap_from_slice( slice, cmp );
        if ( NULL == heap ) {
            slice_free( slice );
        }
    }
    return heap;
}

extern void heap_free( heap_t *heap )
{
    slice_free( heap->slice );
    free( heap );
}

extern void heap_process_items( const heap_t *heap, item_process_fct fct,
                                void *context )
{

    if ( NULL == heap )  return;     // no heap, no items to traverse

    size_t n = _slice_len( heap->slice );
    for ( size_t i = 0; i < n; ++i ) {
        if ( fct( i, _pointer_slice_item_at( heap->slice, i), context ) )
            break;
    }
}

// TODO: add a heap_find( heap_t *heap, void *data )
// which does a search in O(log(n)), stopping as soon as data cannot be further
// in the heap, starting from the closest end... Try to figure out a workable
// solution, following only descendants that can be a solution, not recursive!

/* check children
    calculate children position from parent,
    if left child is out of heap
        return true  (no right child either, end of checking without error)
    if left child has a higher value than parent
        return false (invalid heap property for the left child)
    if right child is in heap
        if it has a higher value than parent
            return false (invalid heap property for the right child)

        make right child parent and recurse.
        if false
            return false (invalid heap property somewhere in its children)

    make left child parent and recurse.
    if false
        return false (invalid heap property somewhere in its children)

    return true (all right and left children have a valid heap property too)
*/
#define HEAP_DEBUG 0
static bool check_children( slice_t *slice, cmp_fct cmp,
                            size_t item_size, size_t n, size_t parent )
{
    size_t left = 2 * parent + 1;       // find left child position
    if ( left >= n )    return true;

    int res = cmp( _pointer_slice_item_at(slice, parent),
                   _pointer_slice_item_at(slice, left) );
#if HEAP_DEBUG
    printf( "Checking heap property for parent=%ld, left child=%ld property:%d\n",
            parent, left, res );
#endif
    if ( res < 0 ) return false;        //invalid heap property for left

    size_t right = left + 1;            // and right child position

    if ( right < n ) {
        int res = cmp( _pointer_slice_item_at(slice, parent),
                       _pointer_slice_item_at(slice, right) );
#if HEAP_DEBUG
        printf( "Checking heap property for parent=%ld, right child=%ld property:%d\n",
                parent, right, res );
#endif
        if ( res < 0 ) return false;    // invalid heap property for right

        if ( false == check_children( slice, cmp, item_size, n, right ) )
            return false;
    }
    if ( false == check_children( slice, cmp, item_size, n, left ) )
        return false;
#if HEAP_DEBUG
    printf( ">> heap property is true for parent %ld\n", parent );
#endif
    return true;
}

// check children starting from root
extern bool heap_check( const heap_t *heap )
{
    if ( NULL == heap )
        return false;   // no a heap is not a valid heap

    size_t n = _slice_len( heap->slice );
    if ( n < 2 )
        return true;    // empty heap or single element heap are always valid

    size_t item_size = _slice_item_size( heap->slice );
    return check_children( heap->slice, heap->cmp, item_size, n, 0 );
}
