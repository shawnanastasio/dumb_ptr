#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "dumb_ptr.h"

struct test {
    int val1;
};

void test_destructor(void *test) {
    printf("Destructing test struct!\n");
}

void print_val1(shared_ptr_t *ptr_);

int main() {
    // Declare a shared ptr variable on the stack.
    // The RAII_SHARED_PTR specifier is a compiler-specific directive that causes
    // the shared_ptr destructor to be called when the variable goes out of scope
    shared_ptr_t test_ptr RAII_SHARED_PTR;
    // Call the shared ptr's constructor and pass it the
    // raw pointer of some heap memory representing a test struct, as well as
    // (optionally) a destructor to call before the struct is free'd.
    bool res = shared_ptr_init(&test_ptr, malloc(sizeof(struct test)), test_destructor);
    if (!res) {
        printf("Failed to create shared_ptr!\n");
        return 1;
    }

    // We can now access the encapsulated test struct using the D(type, shared_ptr)
    // macro. We have to specify the type since C doesn't have a way of storing that
    // information.
    D(struct test, test_ptr).val1 = 100;

    // We can also pass the pointer to other functions that need to use it.
    // These functions will have to manually copy the pointer to a local RAII variable
    // before using it, though.
    print_val1(&test_ptr);

    printf("returned from print_val1!\n");

    // test_ptr goes out of scope and will be destructed here
}

void print_val1(shared_ptr_t *ptr_) {
    // We must copy the shared ptr passed to us to a stack variable before using it
    shared_ptr_t ptr RAII_SHARED_PTR;
    shared_ptr_copy(&ptr, ptr_);

    // We can now use the local shared ptr using the D macro as usual
    printf("The value of val1 is: %d\n", D(struct test, ptr).val1);

    // At the end of the function as ptr goes out of scope, its destructor will be called
}