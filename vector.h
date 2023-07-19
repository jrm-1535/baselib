
#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/*
    Vectors are dynamic arrays. They provide constant read/update access time
    in O(1) within the array, while appending items with an amortized constant
    time in O(1). Vectors provide a reference count to allow sharing them in
    higher level data structures, such as slices.

    Important notes:

    To be type agnostic the interface takes an item value as a void pointer,
    pointing to the actual value and uses the item_size value as the object
    size, which means that if the item is a char for example, the vector is
    created with an item_size of sizeof(char) and an item is passed as:
            data = (void *)&char_value.

    Similarly, vector_item_at returns a void pointer to the value in the
    vector, which can be cast back to a char, such as:
            char_value = *(char *)vector_item_at( vector, index );

    In case the vector contains pointer to objects, a more convenient
    interface, pointer_vector_xxx, is provided to avoid double indirections
    (pointer to pointers). In this case, the objects pointed to are NOT freed
    automatically when the vector is freed. This has to be done before hand by
    freeing each pointer manually:

        for ( size_t i = 0; i < vector_cap( s ); ++i ) {
            free( pointer_vector_item_at( s, i ) );
        }
*/

typedef struct vector vector_t;

// create a new vector of number items, each of item_size size. The number of
// items may be rounded up. The data area is uninitalized.
extern vector_t *new_vector( size_t item_size, size_t number );

// create a new vector of number items, each of item_size size.The number of
// items may be rounded up. The data area is initalized with a copy of the
// given data.
extern vector_t *new_vector_from_data( const void *data,
                                       size_t item_size, size_t number );

// set all vector elements to 0 (or NULL if pointers).
extern void vector_zero( vector_t *vector );

// set all elements of the segment defined by start and len to 0 (or NULL);
extern void vector_segment_zero( vector_t * vector, size_t start, size_t len );

// delete a vector if its reference count is 1, otherwise just decrement its
// reference counter. If vector items are pointers to objects, those objects
// are not freed when the vector is deleted.
extern void vector_free( vector_t *vector );

// let multiple objects share a vector by incrementing its reference counter.
// Note that this is not multi-thread safe.
extern void vector_share( vector_t *vector );

// how many references point to the same vector?
extern int vector_references( const vector_t *vector );

// return the item size 0r 0 if the argument vector is NULL.
extern size_t vector_item_size( const vector_t * vector );

// return the current vector capacity, i.e. how may items can written in it.
extern size_t vector_cap( const vector_t *vector );

// grow the vector, increasing its capacity by allocating a new data area.
// If the vector was shared (reference counter > 1) then a new non-shared
// vector is created and returned. Otherwise the same vector is returned.
extern vector_t *vector_grow( vector_t *vector );

// provide a pointer to the value at a given index in the vector. Return NULL
// if the index is equal or larger than its length.
extern void * vector_item_at( const vector_t *vector, size_t index );

// In the special case of a vector containing a C string (zero terminated char
// array), as a convenience vector_read_string provides a direct access to the
// internal string (it is equivalent to (char *)vector_item_at( vector, 0 )).
extern char *vector_read_string( const vector_t *vector );

// write an item in the vector at position given by index. If the index is
// not in the vector range, it return -1, ortherwise the item is written and
// it returns 0.
extern int vector_write_item_at( vector_t *vector, size_t index,
                                 const void * data );

// function called by vector_process_item for each item from vector[start] to
// vector[start+len-1), unless the function returns true, in which case
// vector_process_items stops immediately. The index argument is the segment
// index, from 0 to len-1, and data is a pointer to the corresponding item
// value in the vector, The argument context comes from the vector_process_item
// caller.
typedef bool (*item_process_fct)( size_t index, void *data, void *context );

// process a segment [start, start + len - 1] in vector by calling the function
// fct passed as argument. Processing the segment stops as soon as fct returns
// true, or at the end of the segment.
extern void vector_process_items( const vector_t *vector, item_process_fct fct,
                                  size_t start, size_t len, void * context );

// The following functions are specific of vectors containing pointers to
// objects in memory. They will fail if the item size is not sizeof(void *).
// They are provided as a convenience.

// Similar to vector_item_at for pointer vectors. The returned value is the
// pointer value, not the pointer to pointer value as it would be with a call
// to vector_item_at.
extern void * pointer_vector_item_at( const vector_t *vector, size_t index );

// Similar to vector_write_item_at for pointer vectors. The argument data is
// the pointer to write, not a pointer to the pointer as it would be with a
// call to vector_write_item_at.
extern int pointer_vector_write_item_at( vector_t *vector, size_t index,
                                         const void *data );

// Similar to vector_process_items for pointer vectors. The data passed to the
// item_process_fct is the pointer in the vector, not a pointer to the pointer
// as it would be with a call to vector_process_items.
extern void pointer_vector_process_items( const vector_t *vector,
                                          item_process_fct fct,
                                          size_t start, size_t len,
                                          void * context );

#endif /* __VECTOR_H__ */
