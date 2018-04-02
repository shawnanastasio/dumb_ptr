#pragma once

/**
 * This file is a part of the dumb_ptr project. See https://github.com/shawnanastasio/dumb_ptr
 * Copyright (C) 2018 Shawn Anastasio
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef __GNUC__
    #error "You need a compiler that supports GNU C extensions like gcc or clang to use dumb_ptr!"
#endif

typedef void (*destructor_t)(void *);

// A pseudo-class representing a shared pointer, meant to mimic C++11's std::shared_ptr
typedef struct shared_ptr {
    uint32_t *refcount;
    void *ptr;
    destructor_t ptr_destructor;
} shared_ptr_t;

/**
 * The constructor for a shared pointer. Must be called explicitly
 * @param this   pointer to shared_ptr_t struct to initalize. MUST BE A STACK VARIABLE WITH THE RAII_SHARED_PTR ATTRIBUTE!
 * @param ptr    pointer to heap memory to manage. MUST BE ABLE TO BE FREED WITH free()! Will be freed when the refcount reaches 0
 * @param ptr_destructor  (optional) destructor function to be called when refcount reaches 0 before free.
 * @return bool  true on success, false on heap allocation error.
 */
static inline bool shared_ptr_init(shared_ptr_t *this, void *ptr, destructor_t ptr_destructor) {
    this->ptr = ptr;
    this->ptr_destructor = ptr_destructor;

    this->refcount = malloc(sizeof(uint32_t));
    if (!this->refcount)
        return false;
    *this->refcount = 1;

    return true;
}

/**
 * The copy constructor for a shared pointer. Must be called explicitly. This will increment the reference count.
 * @param this  pointer to shared_ptr_t struct to initalize. MUST BE A STACK VARIABLE WITH THE RAII_SHARED_PTR ATTRIBUTE!
 * @param that  pointer to another shared_ptr_t to make a copy of.
 */
static inline void shared_ptr_copy(shared_ptr_t *this, shared_ptr_t *that) {
    this->ptr = that->ptr;
    this->refcount = that->refcount;
    *this->refcount += 1;
}

/**
 * The destructor for a shared pointer. NOT TO BE CALLED EXPLICITLY. Will be called automatically when the
 * shared_ptr leaves the scope, provided it was declared with RAII_SHARED_PTR. Will decrement refcount and
 * free the heap pointer if the refcount equals 0.
 * @param pointer to shared_ptr object to destruct.
 */
static inline void shared_ptr_cleanup(shared_ptr_t *this) {
    *this->refcount -= 1;
    if (*this->refcount == 0) {
        free(this->refcount);
        // If a custom destructor was provided, call it
        if (this->ptr_destructor) {
            this->ptr_destructor(this->ptr);
        }

        // Free the pointer
        free(this->ptr);
    }
}

// Compiler-specific macro for specifying a destructor function
// Only works on GCC/Clang afaik
#define RAII(destructor) __attribute__((cleanup(destructor)))
#define RAII_SHARED_PTR RAII(shared_ptr_cleanup);

/**
 * Macro to dereference a smart pointer
 * @param T  the type to interpret the raw pointer as
 * @param p  the smart pointer object that contains the raw pointer
 * Usage:
 *  smart_ptr_t p RAII_SHARED_PTR;
 *  smart_ptr_init(&p, malloc(sizeof(int)), NULL)
 *  D(int, p) = 100; // set the heap memory to the value 100
 */
#define D(T, p) (*((T *)(p).ptr))
