dumb_ptr
========
`dumb_ptr` is a partial implementation of C++11 smart pointers in C (+ GNU extensions). It is meant as a
proof of concept and should probably not be used in anything serious.

So far, it has been confirmed to work on recent GCC and Clang.

What?
-----
C++11 introduced a number of wrapper classes for pointers called "smart pointers" such as `shared_ptr`, `unique_ptr` and `weak_ptr`. These classes allow programmers to automatically manage heap memory without having to worry about memory leaks, as all heap memory owned by them is automatically freed when the pointer goes out of scope. This pattern is often called [RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization).

The C language doesn't have any similar constructs so programmers must manually manage memory allocations and frees. This often leads to memory leaks as all code paths that lead to a return must free any memory previously allocated by the function.

However, by (ab)using GNU extensions to the C language, it is possible to mimic C++11's behavior and reduce the possibility of memory leaks. Specifically, the [GNU Common Variable Attribute](https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html) `cleanup` which allows the user to specify a
cleanup function that is called when a given variable goes out of scope. By creating a pseudo-class that
encapsulates a heap pointer and declaring it with this attribute, we can make sure that whenever it goes out of
scope the heap data it owns can be freed.

For now only `shared_ptr` is implemented, as `unique_ptr` and `weak_ptr` are only useful when the language
can enforce restrictions on their usage (which C won't).

Demonstration
-------
In C++11, a common usage of a `shared_ptr` would look like the following:
```C++
bool f1() {
    std::shared_ptr<int> i = std::make_shared<int>(100);
    f2(i);
    std::cout << *i;

    if (function_that_could_fail() == false) {
        // If we return from the function early, `i` will still be destructed and there will be no memory leak
        return false;
    }

    // At the end of the function, as `i` is destructed and the reference count goes to 0,
    // the int on the heap gets deallocated
    return true;
}

void f2(std::shared_ptr<int> copy) {
    // Here, copy contains a copy of the `i` shared_ptr which can be used to access the heap variable.
    // It also incremented the reference count for the heap variable.
    *copy += 100;

    // Here, copy is destructed and the reference count for the heap variable is decremented.
}
```
Using this strategy, the heap variable pointed to by `i` never gets leaked as the `shared_ptr`'s destructor is called automatically on all code paths that end in a function return.

In idiomatic C without `dumb_ptr`, the above code would look something like this (error handling omitted for brevity's sake):
```C
bool f1() {
    bool result;
    int *i = malloc(sizeof(int));
    *i = 100;
    f2(i);
    printf("%d", *i);

    if (function_that_could_fail() == false) {
        result = false;
        goto end;
    }

    result = true;
end:
    free(i);
    return result;
}

void f2(int *i) {
    *i += 100;
}
```
This (perhaps slightly contrived) example demonstrates how managing heap memory in C has the potential to introduce issues. For example, had we returned from inside the `if` block, the memory pointed to by `i` would be
leaked and impossible to recover. We could have added a second call to `free` inside the `if` block, but
then we would have to remember to include `free` calls for each and every heap variable we allocate in
every place we return, which introduces obvious maintainability issues.

Using `dumb_ptr`, we can mimic the C++11 style of doing things and remove the possibility of memory leaks (again, error checking is omitted):
```C
#include "dumb_ptr.h"

bool f1() {
    // Create a shared_ptr_t and initalize it to handle a chunk of memory from malloc
    // Also, specify no special destructor function (NULL)
    shared_ptr_t i RAII_SHARED_PTR;
    shared_ptr_init(&i, malloc(sizeof(int)), NULL);

    // Dereferencing of the shared_ptr is done through the D(type, ptr) macro
    D(int, i) = 100;

    // Functions can accept a raw pointer to a shared_ptr, but they must copy it
    // explicitly before use
    f2(&i);

    printf("%d", D(int, i));

    if (function_that_could_fail() == false) {
        // We can return from here and `i`'s destructor will be called automatically
        // and free the malloc'd memory.
        return false;
    }

    // We can return here too
    return true;
}

void f2(shared_ptr_t *i) {
    // Since C doesn't have a mechanism for automatically creating copies, we have
    // to do it manually to maintain the integrity promises of shared_ptr_t
    shared_ptr_t copy RAII_SHARED_PTR;
    shared_ptr_copy(&copy, i);

    // We can again dereference the pointer using the D() macro
    D(int, copy) += 100;

    // As with C++, copy will be destructed here and the refcount will be decremented
}
```
As you can see, using dumb_ptr lets us approximate C++11's smart pointers and removes a lot of the boilerplate
required for manual memory management in traditional C. In spite of this however, there are still some things
that the user must remember to do in order to maintain the integrity promises that `dumb_ptr` offers.

Firstly, all local `shared_ptr_t` variables must be declared with the `RAII_SHARED_PTR` specifier which is a
macro that applies the `cleanup` attribute to the variable with the shared_ptr destructor.

Secondly, all functions that accept a raw pointer to a `shared_ptr_t` must explicitly copy it
to a local variable with the `RAII_SHARED_PTR` attribute using the `shared_ptr_copy` function. This ensures
that the reference count for the shared_ptr is incremented and decremented accordingly so that it is not
wrongfully destroyed.

Thirdly, the programmer must keep track of the type stored in a shared_ptr and provide the correct type
when dereferencing with D().

Finally, the programmer must ensure that the heap memory passed to `shared_ptr_init` is valid and can be
free'd with the stdlib's `free` function. This includes checking the `malloc` result to be non-NULL which
was not done in the above example code. Since shared_ptr_init itself allocates some heap memory for the
reference count, you must also check the result of that function call to be `true`.

A destructor may also be provided to `shared_ptr_init` that will be called with the heap raw pointer before it is freed.

Usage
-----
`dumb_ptr` is distributed as a single header file, so all you have to do is drop it in to your project and use it
as described in the above example. You can also see the `example.c` file for a more complete example.

License
-------
`dumb_ptr` is provided under the terms of the MIT license. Please view `LICENSE` for more information.
