
#ifndef __SLICE_H__
#define __SLICE_H__

#include <stddef.h>
#include "vector.h"
/*
    Slices are dynamic arrays or vectors (arrays that grow automatically when
    they get full, and provide amortized access time in O(1)).
    A slice has a current length and capacity. It is possible to get and set
    item values within the segment [0,length[ and to append one item to the
    slice, i.e. to first increment the length and set the last item value.
    Capacity is the size currently available within the current array, which
    might change any time an item is appended to the slice. Multiple slices can
    share the same array, which means that the array value can change for all
    those slices whenever an item is set through one of the slices.

    array:   0 1 2 3 4 5 6 7 8 9 a b c d e f size=16
            [I I I I I I I I I I I I I I I I]
    slice:           ^               ^
                   start=4         beyond=12
                     |<------------->|         length=(beyond-start)=8
                     |<-------------------->|  capacity=(size-start)=12

    Important notes:

    To be type agnostic the interface takes an item value as a void pointer,
    pointing to the actual value and uses the item_size value as the object
    size, which means that if the item is a char for example, the slice is
    created with an item_size of sizeof(char) and an item is passed as:
            data = (void *)&char_value.

    Similarly, slice_item_at returns a void pointer to the value in the
    slice, which can be cast back to a char, such as:
            char_value = *(char *)slice_item_at( slice, index );

    In case the slice contains pointer to objects, a more convenient
    interface, pointer_slice_xxx, is provided to avoid double indirections
    (pointer to pointers). In this case, the objects pointed to are NOT freed
    automatically when the slice is freed by calling slice_free.

    This can be done before hand by freeing each pointer manually:

        for ( size_t i = 0; i < slice_len( s ); ++i ) {
            free( pointer_slice_item_at( s, i ) );
        }

    or by calling instead pointer_slice_free( s ), which does free each
    pointer before freeing the whole slice.
*/

typedef struct slice slice_t;

// create a new slice with a new array of number items, each of item_size size.
// Initially, start=0, length=0 (beyond=start) and capacity=number.
extern slice_t *new_slice( size_t item_size, size_t number );

// create a new slice with a new array of number items, each of item_size size.
// The slice is created with start=0, length=number and capacity=number, and
// the given data area is copied in the array.
extern slice_t *new_slice_from_data( const void *data,
                                     size_t item_size, size_t number );

// create a new slice with the given vector. The slice is created with start=0,
// length=number and capacity=vector_cap( vector ). The vector is now managed
// by the slice, and will atomatically grow as needed by the slice. It will be
// deleted when the slice is deleted, unless it is shared with other objects.
extern slice_t *new_slice_with_vector( vector_t *vector, size_t number );

// create a new slice sharing its array with another slice. The start and beyond
// parameters give the segment of the shared array that the new slice gets. The
// first accessible array item is at start and the last one is at beyond - 1.
// The valid values for both start and beyond are between 0 and the existing
// slice length.
extern slice_t *new_slice_from_slice( const slice_t *slice,
                                      size_t start, size_t beyond );

// create a new slice identical to the given slice. The newly created slice
// shares its array with the original slice, and has the same start and length
// as the original slice. Modifying any of the two slice may increase the
// shared array size, which will result in making a new array for one of the
// slices. If however the current array size is sufficient, then the same array
// stays shared by the two slices. SInce the array reference count is
// incremented when the new slice is created, it is possible to free each of
// those slices without interfering with the other.
extern slice_t *slice_dup( const slice_t * slice );

// set all items in the slice [0..len-1] to 0 or NULL pointers.
extern void slice_zero( slice_t *slice );

// set the privately defined use field, and return its previous value, If that
// field points to allocated memory, it must be freed before calling slice_free
extern void *slice_set_use( slice_t *slice, void *use );

// return the privately defined use field.
extern void *slice_get_use( slice_t *slice );

// re-slice a slice by changing its length, as long as it stays in [0..cap].
extern int slice_update_len( slice_t *slice, size_t len );

// return the current slice capacity
extern size_t slice_cap( const slice_t *slice );

// return the current slice length
extern size_t slice_len( const slice_t *slice );

// return a pointer to the slice start item and the number of following items
// (i.e. the slice length)
extern void *slice_data_n_len( const slice_t *slice, size_t *lenp );

// return the item size or 0 if the argument slice is NULL.
extern size_t slice_item_size( const slice_t * slice );

// return a void pointer pointing to the item at position given by index
extern void *slice_item_at( const slice_t *slice, size_t index );

// slice_write_item_at makes a copy of the data pointed to by the void pointer
// data into the slice array at the given index. The index must be less than
// the current slice length to succeed (use instead slice_append_item to write
// the item at the current length). It returns 0 in case of success or -1 if
// slice is invalid or index is not in range.
extern int slice_write_item_at( slice_t *slice,
                                size_t index, const void *data );

// slice_insert_item_at extends the current slice length by one and moves up
// the content of the slice starting at index till the end of the slice in
// order to make room for the new data. It then writes a copy of the data
// pointed to by the void pointer data into the slice array at the given index.
// If the given index is the current length it behaves as slice_append_item.
// It returns -1 if index is bigger then the current length, otherwise 0.
extern int slice_insert_item_at( slice_t *slice, size_t index,
                                 const void *data );

// slice_remove_item_at compacts the slice items by moving all items above the
// index 1 item back and then shrinks the current slice length by one. It
// returns 0 in case of success or -1 otherwise,
extern int slice_remove_item_at( slice_t *slice, size_t index );

// slice_move_items_from modifies a slice by moving all items between index and
// index+len-1 to the new position between index+offset and index+len-1+offset.
// This will overwrite |offset| items, which may need to be temporarily saved
// by the caller before, if they are needed. It returns 0 in case of success
// or -1 otherwise,
extern int slice_move_items_from( slice_t *slice,
                                  size_t index, size_t len, ssize_t offset );

// slice_swap_items exchanges the values of 2 items. It returns 0 in case of
// success or -1 if any item index is out of range
extern int slice_swap_items( slice_t *slice, size_t index1, size_t index2 );

// append an item to the end of the slice, i.e. at start + length. The length
// is first incremented, which migh require allocating more space to the array.
// In that case, the slice capacity is increased as well. It returns 0 in case
// of success or -1 otherwise.
extern int slice_append_item( slice_t *slice_t, const void *data );

// free the slice. If the underlying array is not shared with any other slice,
// it is deleted as well, otherwise its reference count is just decremented. It
// returns 0 in case of success, or -1 otherwise (bad slice argument).
extern int slice_free( slice_t *slice );

// slice_process_items processes all items in slice by calling the function
// fct (item_process_fct is defined in vector.h) passed as argument.
// Processing the slice stops as soon as fct returns true, or at the end of
// the slice.
extern void slice_process_items( const slice_t *slice, item_process_fct fct,
                                 void *context );

// slice_sort_items sorts all items in slice by calling qsort with the given
// comparison fonction cmp. It returns true if the slice was sorted or false
// if the argument slice or argument cmp are NULL.
typedef int (*comp_fct)( const void *item1, const void *item2 );
extern bool slice_sort_items( slice_t *slice, comp_fct cmp );

// The following functions are specific of slices containing pointers to
// objects in memory. They will fail if the item size is not sizeof(void *).
// They are provided as a convenience.

// Similar to slice_item_at for pointer slices. The returned value is the
// pointer value, not the pointer to pointer value as it would be with a call
// to slice_item_at.
extern void * pointer_slice_item_at( const slice_t *slice, size_t index );

// Similar to slice_write_item_at for pointer slices. The argument data is
// the pointer to write, not a pointer to the pointer as it would be with a
// call to slice_write_item_at.
extern int pointer_slice_write_item_at( slice_t *slice, size_t index,
                                        const void *data );

// similar to slice_insert_item_at for pointer slices.  The argument data is
// the pointer to write, not a pointer to the pointer as it would be with a
// call to slice_insert_item_at.
extern int pointer_slice_insert_item_at( slice_t *slice, size_t index,
                                         const void *data );

// Similar to slice_append_item for pointer slices. The argument data is the
// pointer to append, not a pointer to the pointer as it would be with a call
// to slice_append_item.
extern int pointer_slice_append_item( slice_t *slice_t, const void *data );

// Similar to slice_process_items for pointer slices. The data passed to the
// item_process_fct is the pointer in the slice, not a pointer to the pointer
// as it would be with a call to vector_process_items.
extern void pointer_slice_process_items( const slice_t *slice,
                                         item_process_fct fct,
                                         void * context );

// similar to slice_free except that it first passes all pointers in the slice
// to the function f before freeing the slice. It is intended for freeing
// slices containing complex objects that must be cleaned before disposal.
extern int pointer_slice_finalize( slice_t *slice, void (*f)( void *) );

// similar to slice_free except that it first frees all pointers in the slice
// before freeing the slice. It is equivalent to calling pointer_slice_finalize
// on the slice with the standard free function.
extern int pointer_slice_free( slice_t *slice );

#endif /* __SLICE_H__ */
