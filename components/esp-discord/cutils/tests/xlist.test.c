#include "xlist.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char* name;
} person_t;

static person_t* john = NULL;
static person_t* marry = NULL;

static void free_data(void* data) {
    person_t* person = (person_t*) data;

    if(!person) {
        return;
    }

    free(person->name);
    free(person);

    if(person == john)       { john = NULL; }
    else if(person == marry) { marry = NULL; }
}

static void test_dynmem() {
    xlist_t list = NULL;
    assert(xlist_create(&(xlist_config_t) {
        .data_free_handler = &free_data
    }, &list) == CU_OK);
    assert(xlist_is_empty(list));
    assert(xlist_vadd(list, john = cu_ctor(person_t, .name = strdup("John"))) == 1);
    assert(!xlist_is_empty(list));
    assert(xlist_vadd(list, marry = cu_ctor(person_t, .name = strdup("Marry"))) == 2);
    assert(!xlist_is_empty(list));
    assert(xlist_size(list) == 2);
    assert(xlist_remove_data(list, john) == 1);
    assert(!john);
    assert(marry);
    assert(xlist_size(list) == 1);
    assert(xlist_remove_data(list, marry) == 1);
    assert(!john);
    assert(!marry);
    assert(xlist_size(list) == 0);
    assert(xlist_flush(list) == 0);
    assert(xlist_vadd(list, john = cu_ctor(person_t, .name = strdup("John"))) == 1);
    assert(xlist_vadd(list, marry = cu_ctor(person_t, .name = strdup("Marry"))) == 2);
    assert(john && marry);
    assert(xlist_flush(list) == 2); // free both persons
    assert(!john && !marry);
    assert(xlist_destroy(list) == CU_OK); // vdestroy because it's already flushed

    list = NULL;
    assert(xlist_create(&(xlist_config_t) {
        .data_free_handler = &free_data
    }, &list) == CU_OK);
    assert(xlist_vadd(list, john = cu_ctor(person_t, .name = strdup("John"))) == 1);
    assert(xlist_vadd(list, marry = cu_ctor(person_t, .name = strdup("Marry"))) == 2);
    assert(john && marry);
    assert(xlist_destroy(list) == CU_OK); // custom free fnc
    assert(!john && !marry);
}

static void test_get() {
    xlist_t list = NULL;
    int num = 5;
    assert(xlist_create(NULL, &list) == CU_OK);
    assert(xlist_vadd(list, &num) == 1);
    assert(xlist_vadd(list, &num) == 2);
    assert(xlist_vadd(list, &num) == 3);
    assert(xlist_vadd(list, &num) == 4);
    assert(xlist_vadd(list, &num) == 5);
    assert(xlist_vadd(list, &num) == 6);
    assert(xlist_vadd(list, &num) == 7);
    assert(xlist_vadd(list, &num) == 8);
    assert(xlist_vadd(list, &num) == 9);
    assert(xlist_vadd(list, &num) == 10);
    assert(xlist_size(list) == 10);

    int* data;
    assert(xlist_get_tdata(list, 8, int*, data) == CU_OK);
    assert(*data == 5);
}

int main() {
    test_get();
    test_dynmem();

    xlist_t list = NULL;
    assert(xlist_size(list) == CU_ERR_INVALID_ARG); // list is null

    assert(xlist_create(NULL, &list) == CU_OK);
    assert(xlist_size(list) == 0);

    int a = 5;
    int b = 4;
    int c = 6;
    void* data = NULL;

    assert(xlist_vadd(list, &a) == 1);
    assert(xlist_size(list) == 1);
    assert(xlist_get_data(list, 1, &data) == CU_ERR_NOT_FOUND);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_remove_data(list, data) == 1); // remove a
    assert(xlist_size(list) == 0);
    assert(xlist_get_data(list, 0, &data) == CU_ERR_NOT_FOUND);

    assert(xlist_vadd(list, &a) == 1);
    assert(xlist_vadd(list, &a) == 2); // add a again
    assert(xlist_size(list) == 2);
    assert(xlist_get_data(list, 1, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_remove_data(list, data) == 2); // delete both occurrences
    assert(xlist_size(list) == 0);

    assert(xlist_vadd(list, &a) == 1);
    assert(xlist_vadd(list, &b) == 2);
    assert(xlist_size(list) == 2);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_get_data(list, 1, &data) == CU_OK);
    assert(*(int*) data == b);
    assert(xlist_remove_data(list, data) == 1); // remove b
    assert(xlist_size(list) == 1);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_get_data(list, 1, &data) == CU_ERR_NOT_FOUND);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(xlist_remove_data(list, data) == 1); // remove a
    assert(xlist_size(list) == 0);

    assert(xlist_vadd(list, &a) == 1);
    assert(xlist_vadd(list, &b) == 2);
    assert(xlist_vadd(list, &c) == 3);
    assert(xlist_size(list) == 3);
    assert(xlist_flush(list) == 3); // remove all
    assert(xlist_size(list) == 0);

    assert(xlist_vadd(list, &a) == 1);
    assert(xlist_vadd_to_front(list, &b) == 2);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == b); // b should be in the front

    // test references

    xnode_t node = NULL;
    xnode_t node_tmp = NULL;
    int x = 10;
    assert(xlist_add_to_front(list, &x, &node) == 3);
    assert(xlist_get(list, 0, &node_tmp) == CU_OK);
    assert(node == node_tmp);

    assert(xlist_destroy(list) == CU_OK);

    return 0;
}